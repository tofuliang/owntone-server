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

    <template v-else-if="album">
      <div class="columns">
        <!-- Album Cover -->
        <div class="column is-narrow">
          <figure class="image is-square" style="width: 300px;">
            <img
              :src="album.artwork_url || null"
              :alt="album.name"
              class="has-background-light"
            />
          </figure>
        </div>

        <!-- Album Info -->
        <div class="column">
          <div class="content">
            <h1 class="title">{{ album.name }}</h1>
            <p class="subtitle">
              <router-link
                :to="{ name: 'music-opensubsonic-artist', params: { id: album.artist_id } }"
                class="has-text-weight-bold"
              >
                {{ album.artist }}
              </router-link>
            </p>
            <p class="album-info">
              <span v-if="album.year">{{ album.year }}</span>
              <span v-if="album.genre">• {{ album.genre }}</span>
              <span v-if="album.track_count">
                • {{ album.track_count }} tracks
              </span>
            </p>
          </div>

          <!-- Action Buttons -->
          <div class="buttons">
            <button class="button is-primary" @click="playAlbum">
              <span class="icon">
                <i class="mdi mdi-play"></i>
              </span>
              <span>{{ $t('actions.play') }}</span>
            </button>
            <button class="button" @click="shuffleAlbum">
              <span class="icon">
                <i class="mdi mdi-shuffle"></i>
              </span>
              <span>{{ $t('actions.shuffle') }}</span>
            </button>
            <button class="button" @click="addToQueue">
              <span class="icon">
                <i class="mdi mdi-plus"></i>
              </span>
              <span>{{ $t('actions.add') }}</span>
            </button>
          </div>
        </div>
      </div>

      <!-- Track List -->
      <div v-if="album.tracks?.length" class="track-list">
        <div class="list">
          <ListItem
            v-for="(track, index) in album.tracks"
            :key="track.id"
            :lines="[
              track.title,
              formatTrackInfo(track, index + 1)
            ]"
            @open="playSong(track)"
            @open-details="openTrackDetails(track)"
          >
            <template #before>
              <div class="track-number has-text-grey">{{ index + 1 }}</div>
            </template>
            <template #after>
              <div class="track-duration has-text-grey">{{ formatDuration(track.length_ms / 1000) }}</div>
            </template>
          </ListItem>
        </div>
      </div>

      <div v-else class="content has-text-centered">
        <p>{{ $t('page.opensubsonic.album.no_tracks') }}</p>
      </div>
    </template>

    <div v-else class="content has-text-centered">
      <p>{{ $t('page.opensubsonic.album.not_found') }}</p>
    </div>
  </ContentWithHeading>

  <!-- Track Details Modal -->
  <modal-dialog-track
    :item="selectedTrack"
    :show="showTrackDetailsModal"
    @close="showTrackDetailsModal = false"
  />
</template>

<script>
import { computed, onMounted, ref } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { useOpensubsonicStore } from '@/stores/opensubsonic'
import ContentWithHeading from '@/templates/ContentWithHeading.vue'
import ListItem from '@/components/ListItem.vue'
import LoaderListItem from '@/components/LoaderListItem.vue'
import ModalDialogTrack from '@/components/ModalDialogTrack.vue'

export default {
  name: 'PageAlbumOpensubsonic',

  components: {
    ContentWithHeading,
    ListItem,
    LoaderListItem,
    ModalDialogTrack
  },

  setup() {
    const route = useRoute()
    const router = useRouter()
    const store = useOpensubsonicStore()

    const album = computed(() => store.currentAlbum)
    const loading = computed(() => store.loading)
    const error = computed(() => store.error)

    // Modal dialog state
    const selectedTrack = ref({})
    const showTrackDetailsModal = ref(false)

    const formatTrackInfo = (track, trackNumber) => {
      const parts = []
      if (track.artist && track.artist !== album.value?.artist) {
        parts.push(track.artist)
      }
      if (track.genre && track.genre !== album.value?.genre) {
        parts.push(track.genre)
      }
      return parts.join(' • ') || `Track ${trackNumber}`
    }

    const formatDuration = (seconds) => {
      if (!seconds) return '--:--'
      const minutes = Math.floor(seconds / 60)
      const remainingSeconds = seconds % 60
      return minutes + ':' + String(remainingSeconds).padStart(2, '0')
    }


    const playAlbum = async () => {
      if (!album.value?.tracks?.length) return
      const tracks = album.value.tracks.map(track => 
        `os:${track.id}:a:${album.value.id}`
      )
      await store.replaceQueueAndPlay(tracks)
    }

    const shuffleAlbum = async () => {
      if (!album.value?.tracks?.length) return
      const tracks = [...album.value.tracks]
        .sort(() => Math.random() - 0.5)
        .map(track => `os:${track.id}:a:${album.value.id}`)
      await store.replaceQueueAndPlay(tracks)
    }

    const addToQueue = async () => {
      if (!album.value?.tracks?.length) return
      const tracks = album.value.tracks.map(track => 
        `os:${track.id}:a:${album.value.id}`
      )
      await store.addToQueue(tracks)
    }

    const playSong = async (track) => {
      const url = `os:${track.id}:a:${album.value.id}`
      await store.addToQueueAndPlay(url)
    }

    const openTrackDetails = (track) => {
      // Convert OpenSubsonic track format to standard track format for modal
      const convertedTrack = {
        ...track,
        // Generate correct URI format for queue operations: os:trackId:a:albumId
        uri: `os:${track.id}:a:${album.value.id}`,
        // Ensure required fields exist with fallbacks
        album_artist: track.album_artist || track.artist,
        album_artist_id: track.album_artist_id || null,
        album_id: track.album_id || null,
        composer: track.composer || '',
        date_released: track.date_released || null,
        genre: track.genre || '',
        disc_number: track.disc_number || 1,
        channels: track.channels || 2,
        samplerate: track.samplerate || 44100,
        time_added: track.time_added || new Date().toISOString(),
        rating: track.rating || 0,
        comment: track.comment || '',
        path: track.path || track.uri || '',
        play_count: track.play_count || 0
      }
      
      selectedTrack.value = convertedTrack
      showTrackDetailsModal.value = true
    }

    const goBack = () => {
      router.back()
    }

    onMounted(async () => {
      // Initialize store if not already done
      if (!store.isConnected && !store.loading) {
        await store.initialize()
      }
      
      // Load album data if connected
      if (store.isConnected) {
        await store.loadAlbum(route.params.id)
      }
    })

    return {
      album,
      loading,
      error,
      selectedTrack,
      showTrackDetailsModal,
      formatTrackInfo,
      formatDuration,
      playAlbum,
      shuffleAlbum,
      addToQueue,
      playSong,
      openTrackDetails,
      goBack
    }
  }
}
</script>

<style scoped>
.track-list {
  margin-top: 2rem;
}

.track-number {
  width: 2rem;
  text-align: right;
  margin-right: 1rem;
}

.track-duration {
  margin-left: 1rem;
}

.album-info {
  color: #666;
}
</style>