<template>
  <ContentWithHeading>
    <template #heading>
      <div class="level">
        <div class="level-left">
          <div class="level-item">
            <button @click="goBack" class="button is-text">
              <span class="icon">
                <i class="mdi mdi-arrow-left"></i>
              </span>
              <span>{{ $t('page.opensubsonic.back') }}</span>
            </button>
          </div>
        </div>
      </div>
    </template>

    <div v-if="loading" class="has-text-centered">
      <LoaderListItem />
    </div>

    <div v-else-if="error" class="notification is-danger">
      {{ error }}
    </div>

    <template v-else-if="artist">
      <div class="columns">
        <!-- Artist Cover -->
        <div class="column is-narrow">
          <figure class="image is-square" style="width: 300px;">
            <img
              :src="artist.artwork_url || '/logo.svg'"
              :alt="artist.name"
              style="object-fit: cover; width: 100%; height: 100%;"
            />
          </figure>
        </div>
        
        <!-- Artist Info -->
        <div class="column">
          <div class="content">
            <h1 class="title">{{ artist.name }}</h1>
            <p v-if="artist.album_count" class="subtitle">
              {{ $t('data.albums', { count: artist.album_count }) }}
            </p>
          </div>
        </div>
      </div>

      <!-- Albums -->
      <div v-if="artist.albums?.length" class="albums-section">
        <h2 class="subtitle">{{ $t('page.opensubsonic.artist.albums') }}</h2>
        <div class="list">
          <ListItem
            v-for="album in artist.albums"
            :key="album.id"
            :lines="[
              album.name,
              formatAlbumInfo(album)
            ]"
            :image="album.artwork_url ? { url: album.artwork_url, caption: album.name } : null"
            @open="showAlbum(album)"
            @openDetails="showDetails(album, 'album')"
          />
        </div>
      </div>

      <!-- Top Songs -->
      <div v-if="artist.songs?.length" class="songs-section">
        <h2 class="subtitle">{{ $t('page.opensubsonic.artist.top_songs') }}</h2>
        <div class="list">
          <ListItem
            v-for="song in artist.songs"
            :key="song.id"
            :title="song.title"
            :subtitle="formatSongInfo(song)"
            :image="song.coverArt ? getCoverArtUrl(song.coverArt) : null"
            @click="playSong(song)"
          />
        </div>
      </div>

      <div v-if="!hasContent" class="content has-text-centered">
        <p>{{ $t('page.opensubsonic.artist.no_content') }}</p>
      </div>
    </template>

    <div v-else class="content has-text-centered">
      <p>{{ $t('page.opensubsonic.artist.not_found') }}</p>
    </div>

    <!-- Album Details Modal -->
    <modal-dialog-playable
      v-if="selectedItem"
      :buttons="modalButtons"
      :item="modalItem"
      :show="showDetailsModal"
      @close="showDetailsModal = false"
    />
  </ContentWithHeading>
</template>

<script>
import { computed, onMounted, ref } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { useI18n } from 'vue-i18n'
import { useOpensubsonicStore } from '@/stores/opensubsonic'
import ContentWithHeading from '@/templates/ContentWithHeading.vue'
import ListItem from '@/components/ListItem.vue'
import LoaderListItem from '@/components/LoaderListItem.vue'
import ModalDialogPlayable from '@/components/ModalDialogPlayable.vue'

export default {
  name: 'PageArtistOpensubsonic',

  components: {
    ContentWithHeading,
    ListItem,
    LoaderListItem,
    ModalDialogPlayable
  },

  setup() {
    const route = useRoute()
    const router = useRouter()
    const { t } = useI18n()
    const store = useOpensubsonicStore()

    // Modal dialog state
    const selectedItem = ref(null)
    const showDetailsModal = ref(false)

    const artist = computed(() => store.currentArtist)
    const loading = computed(() => store.loading)
    const error = computed(() => store.error)

    const hasContent = computed(() => {
      return (
        (artist.value?.albums?.length || 0) +
        (artist.value?.songs?.length || 0) > 0
      )
    })

    const formatAlbumInfo = (album) => {
      const parts = []
      if (album.year) {
        parts.push(album.year)
      }
      if (album.track_count) {
        parts.push(`${album.track_count} tracks`)
      }
      return parts.join(' • ')
    }

    const formatSongInfo = (song) => {
      const parts = []
      if (song.album) {
        parts.push(song.album)
      }
      if (song.duration) {
        parts.push(formatDuration(song.duration))
      }
      return parts.join(' • ')
    }

    const formatDuration = (seconds) => {
      const minutes = Math.floor(seconds / 60)
      const remainingSeconds = seconds % 60
      return minutes + ':' + String(remainingSeconds).padStart(2, '0')
    }


    const showAlbum = (album) => {
      router.push({
        name: 'music-opensubsonic-album',
        params: { id: album.id }
      })
    }

    const goBack = () => {
      router.back()
    }

    // Modal dialog computed properties
    const modalButtons = computed(() => {
      return []
    })

    const modalItem = computed(() => {
      if (!selectedItem.value) return null

      const item = selectedItem.value
      
      if (item.type === 'album') {
        return {
          name: item.name,
          properties: [
            { key: 'property.artist', value: artist.value?.name },
            { key: 'property.year', value: item.year },
            { key: 'property.genre', value: item.genre },
            { key: 'property.tracks', value: item.track_count ? `${item.track_count} tracks` : null }
          ],
          uri: `os:album:${item.id}`
        }
      }
      
      return null
    })

    const showDetails = (item, type) => {
      selectedItem.value = { ...item, type }
      showDetailsModal.value = true
    }

    const playSong = async (song) => {
      // For artist page, we don't have albumId, so we'll pass the song without source info
      // The backend will use fallback values
      const url = `os:${song.id}`
      await store.addToQueueAndPlay(url)
    }

    onMounted(async () => {
      console.log('PageArtistOpensubsonic onMounted - Artist ID:', route.params.id)
      console.log('Store isConnected:', store.isConnected)
      console.log('Store loading:', store.loading)
      console.log('Store status:', store.status)
      
      // Initialize store if not already done
      if (!store.isConnected && !store.loading) {
        console.log('Initializing OpenSubsonic store...')
        await store.initialize()
        console.log('Store initialized. isConnected:', store.isConnected)
      }
      
      // Load artist data if connected
      if (store.isConnected) {
        console.log('Loading artist data for ID:', route.params.id)
        await store.loadArtist(route.params.id)
        console.log('Artist loaded:', store.currentArtist)
      } else {
        console.log('Not connected to OpenSubsonic, cannot load artist data')
      }
    })

    return {
      artist,
      loading,
      error,
      hasContent,
      formatAlbumInfo,
      formatSongInfo,
      showAlbum,
      goBack,
      selectedItem,
      showDetailsModal,
      modalButtons,
      modalItem,
      showDetails,
      playSong
    }
  }
}
</script>

<style scoped>
.albums-section,
.songs-section {
  margin-top: 2rem;
}
</style>`