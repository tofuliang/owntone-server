<template>
  <div class="opensubsonic-search">
    <div class="field">
      <div class="control has-icons-left">
        <input
          class="input"
          type="text"
          v-model="query"
          :placeholder="t('page.opensubsonic.search.placeholder')"
          @keyup.enter="search"
        />
        <span class="icon is-left">
          <i class="mdi mdi-magnify"></i>
        </span>
      </div>
    </div>

    <div v-if="loading" class="has-text-centered">
      <LoaderListItem />
    </div>

    <div v-else-if="error" class="notification is-danger">
      {{ error }}
    </div>

    <div v-else-if="searchResults">
      <!-- Artists -->
      <div v-if="searchResults.artists?.length" class="search-section">
        <h2 class="subtitle">{{ t('page.opensubsonic.search.artists') }}</h2>
        <div class="list">
          <ListItem
            v-for="artist in searchResults.artists"
            :key="artist.id"
            :lines="[artist.name, t('page.opensubsonic.artists.album_count', { count: artist.album_count })]"
            :image="getArtistImage(artist)"
            @open="showArtist(artist)"
            @openDetails="showDetails(artist, 'artist')"
          />
        </div>
      </div>

      <!-- Albums -->
      <div v-if="searchResults.albums?.length" class="search-section">
        <h2 class="subtitle">{{ t('page.opensubsonic.search.albums') }}</h2>
        <div class="list">
          <ListItem
            v-for="album in searchResults.albums"
            :key="album.id"
            :lines="[album.name, t('page.opensubsonic.albums.by_artist', { artist: album.artist })]"
            :image="{ url: album.artwork_url, caption: album.name }"
            @open="showAlbum(album)"
            @openDetails="showDetails(album, 'album')"
          />
        </div>
      </div>

      <!-- Songs -->
      <div v-if="searchResults.tracks?.length" class="search-section">
        <h2 class="subtitle">{{ t('page.opensubsonic.search.songs') }}</h2>
        <div class="list">
          <ListItem
            v-for="song in searchResults.tracks"
            :key="song.id"
            :lines="[song.title, `${song.artist} - ${song.album}`]"
            :image="{ url: song.artwork_url, caption: song.title }"
            @open="playSong(song)"
            @openDetails="showDetails(song, 'track')"
          />
        </div>
      </div>

      <div v-if="!hasResults" class="content has-text-centered">
        <p>{{ t('page.opensubsonic.search.no_results') }}</p>
      </div>
    </div>
  </div>

  <!-- Search Result Details Modal -->
  <modal-dialog-track
    v-if="selectedItem && selectedItem.type === 'track'"
    :item="selectedItem"
    :show="showDetailsModal"
    @close="showDetailsModal = false"
  />
  <modal-dialog-playable
    v-else-if="selectedItem && modalItem"
    :buttons="modalButtons"
    :item="modalItem"
    :show="showDetailsModal"
    @close="showDetailsModal = false"
  />
</template>

<script>
import { ref, computed } from 'vue'
import { useRouter } from 'vue-router'
import { useI18n } from 'vue-i18n'
import { useOpensubsonicStore } from '@/stores/opensubsonic'
import ListItem from '@/components/ListItem.vue'
import LoaderListItem from '@/components/LoaderListItem.vue'
import ModalDialogPlayable from '@/components/ModalDialogPlayable.vue'
import ModalDialogTrack from '@/components/ModalDialogTrack.vue'

export default {
  name: 'SearchOpensubsonic',

  components: {
    ListItem,
    LoaderListItem,
    ModalDialogPlayable,
    ModalDialogTrack
  },

  setup() {
    const store = useOpensubsonicStore()
    const router = useRouter()
    const { t, tc } = useI18n()
    const query = ref('')
    const searchResults = computed(() => store.searchResults)
    const loading = computed(() => store.loading)
    const error = computed(() => store.error)

    const hasResults = computed(() => {
      return (
        (searchResults.value?.artists?.length || 0) +
        (searchResults.value?.albums?.length || 0) +
        (searchResults.value?.tracks?.length || 0) > 0
      )
    })

    // Modal dialog state
    const selectedItem = ref(null)
    const showDetailsModal = ref(false)

    const search = async () => {
      if (query.value.trim()) {
        await store.search(query.value.trim())
      }
    }

    const getCoverArtUrl = (id) => {
      return store.getArtworkUrl(id)
    }

    const getArtistImage = (artist) => {
      if (artist.artwork_url) {
        return { url: artist.artwork_url, caption: artist.name }
      }
      return null
    }

    const showArtist = (artist) => {
      router.push({
        name: 'music-opensubsonic-artist',
        params: { id: artist.id }
      })
    }

    const showAlbum = (album) => {
      router.push({
        name: 'music-opensubsonic-album',
        params: { id: album.id }
      })
    }

    const playSong = async (song) => {
      // Use the URI directly from the backend response
      await store.addToQueueAndPlay(song.uri)
    }

    const formatDuration = (seconds) => {
      if (!seconds) return '--:--'
      const minutes = Math.floor(seconds / 60)
      const remainingSeconds = seconds % 60
      return minutes + ':' + String(remainingSeconds).padStart(2, '0')
    }

    // Modal dialog computed properties
    const modalButtons = computed(() => {
      return []
    })

    const modalItem = computed(() => {
      if (!selectedItem.value) return null

      const item = selectedItem.value
      
      if (item.type === 'track') {
        return null
      }
      
      if (item.type === 'artist') {
        return {
          name: item.name,
          properties: [
            { key: 'property.albums', value: t('page.opensubsonic.artists.album_count', { count: item.album_count }) }
          ],
          uri: null
        }
      } else if (item.type === 'album') {
        return {
          name: item.name,
          properties: [
            { key: 'property.artist', value: item.artist },
            { key: 'property.year', value: item.year },
            { key: 'property.genre', value: item.genre },
            { key: 'property.tracks', value: item.track_count ? `${item.track_count} tracks` : null }
          ],
          uri: null
        }
      }
      
      return null
    })

    const showDetails = (item, type) => {
      if (type === 'track') {
        // Convert OpenSubsonic track format to standard track format for ModalDialogTrack
        const convertedTrack = {
          ...item,
          // Generate correct URI format for queue operations: os:trackId:a:albumId
          uri: item.uri,
          // Ensure required fields exist with fallbacks
          album_artist: item.album_artist || item.artist,
          album_artist_id: item.album_artist_id || null,
          album_id: item.album_id || null,
          composer: item.composer || '',
          date_released: item.date_released || null,
          genre: item.genre || '',
          disc_number: item.disc_number || 1,
          channels: item.channels || 2,
          samplerate: item.samplerate || 44100,
          time_added: item.time_added || new Date().toISOString(),
          rating: item.rating || 0,
          comment: item.comment || '',
          path: item.path || item.uri || '',
          play_count: item.play_count || 0,
          media_kind: item.media_kind || 'music',
          data_kind: item.data_kind || 'file',
          type: 'track', // ?????? 'mp3' ?? 'track'
          bitrate: item.bitrate || 320,
          track_number: item.track_number || 1,
          year: item.year || null
        }
        selectedItem.value = convertedTrack
      } else {
        selectedItem.value = { ...item, type }
      }
      showDetailsModal.value = true
    }

    return {
      query,
      searchResults,
      loading,
      error,
      hasResults,
      search,
      getCoverArtUrl,
      getArtistImage,
      showArtist,
      showAlbum,
      playSong,
      t,
      tc,
      selectedItem,
      showDetailsModal,
      modalButtons,
      modalItem,
      showDetails
    }
  }
}
</script>

<style scoped>
.search-section {
  margin-bottom: 2rem;
}

.search-section:last-child {
  margin-bottom: 0;
}
</style>