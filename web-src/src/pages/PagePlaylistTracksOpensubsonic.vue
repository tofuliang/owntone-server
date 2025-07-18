<template>
  <ContentWithHeading>
    <template #heading>
      <div class="level">
        <div class="level-left">
          <div class="level-item">
            <router-link :to="{ name: 'music-opensubsonic' }" class="button is-text">
              <span class="icon">
                <i class="mdi mdi-arrow-left"></i>
              </span>
              <span>{{ $t('page.opensubsonic.back') }}</span>
            </router-link>
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

    <template v-else-if="playlist">
      <div class="columns">
        <!-- Playlist Cover -->
        <div class="column is-narrow">
          <figure class="image is-square" style="width: 300px;">
            <img
              :src="playlist.artwork_url || null"
              :alt="playlist.name"
              class="has-background-light"
            />
          </figure>
        </div>

        <!-- Playlist Info -->
        <div class="column">
          <div class="content">
            <h1 class="title">{{ playlist.name }}</h1>
            <p class="subtitle" v-if="playlist.owner">
              {{ $t('page.opensubsonic.playlists.by_owner', { owner: playlist.owner }) }}
            </p>
            <p class="playlist-info">
              <span v-if="playlist.item_count">
                {{ playlist.item_count }} tracks
              </span>
              <span v-if="playlist.length_ms">
                • {{ formatTotalDuration(playlist.length_ms / 1000) }}
              </span>
              <span v-if="playlist.created">
                • {{ formatDate(playlist.created) }}
              </span>
            </p>
            <p v-if="playlist.comment" class="playlist-comment">
              {{ playlist.comment }}
            </p>
          </div>

          <!-- Action Buttons -->
          <div class="buttons">
            <button class="button is-primary" @click="playPlaylist">
              <span class="icon">
                <i class="mdi mdi-play"></i>
              </span>
              <span>{{ $t('actions.play') }}</span>
            </button>
            <button class="button" @click="shufflePlaylist">
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
      <div v-if="playlist.tracks?.length" class="track-list">
        <div class="list">
          <ListItem
            v-for="(track, index) in playlist.tracks"
            :key="track.id"
            :lines="[
              track.title,
              formatTrackInfo(track, index + 1)
            ]"
            :image="track.artwork_url ? { url: track.artwork_url, caption: track.title } : null"
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
        <p>{{ $t('page.opensubsonic.playlists.empty') }}</p>
      </div>
    </template>

    <div v-else class="content has-text-centered">
      <p>{{ $t('page.opensubsonic.playlists.empty') }}</p>
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
import { useRoute } from 'vue-router'
import { useOpensubsonicStore } from '@/stores/opensubsonic'
import ContentWithHeading from '@/templates/ContentWithHeading.vue'
import ListItem from '@/components/ListItem.vue'
import LoaderListItem from '@/components/LoaderListItem.vue'
import ModalDialogTrack from '@/components/ModalDialogTrack.vue'

export default {
  name: 'PagePlaylistTracksOpensubsonic',

  components: {
    ContentWithHeading,
    ListItem,
    LoaderListItem,
    ModalDialogTrack
  },

  setup() {
    const route = useRoute()
    const store = useOpensubsonicStore()

    const playlist = computed(() => store.currentPlaylist)
    const loading = computed(() => store.loading)
    const error = computed(() => store.error)

    // Modal dialog state
    const selectedTrack = ref({})
    const showTrackDetailsModal = ref(false)

    const formatTrackInfo = (track, trackNumber) => {
      const parts = []
      if (track.artist) {
        parts.push(track.artist)
      }
      if (track.album) {
        parts.push(track.album)
      }
      return parts.join(' • ') || `Track ${trackNumber}`
    }

    const formatDuration = (durationMs) => {
      if (!durationMs) return '--:--'
      const seconds = Math.floor(durationMs / 1000)
      const minutes = Math.floor(seconds / 60)
      const remainingSeconds = seconds % 60
      return minutes + ':' + String(remainingSeconds).padStart(2, '0')
    }

    const formatTotalDuration = (seconds) => {
      if (!seconds) return ''
      const hours = Math.floor(seconds / 3600)
      const minutes = Math.floor((seconds % 3600) / 60)
      if (hours > 0) {
        return `${hours}h ${minutes}m`
      }
      return `${minutes}m`
    }

    const formatDate = (date) => {
      return new Date(date).toLocaleDateString()
    }

    const getCoverArtUrl = (id) => {
      return store.getArtworkUrl(id)
    }

    const playPlaylist = async () => {
      if (!playlist.value?.tracks?.length) return
      const tracks = playlist.value.tracks.map(track => 
        `os:${track.id}:p:${playlist.value.id}`
      )
      await store.replaceQueueAndPlay(tracks)
    }

    const shufflePlaylist = async () => {
      if (!playlist.value?.tracks?.length) return
      const tracks = [...playlist.value.tracks]
        .sort(() => Math.random() - 0.5)
        .map(track => 
          `os:${track.id}:p:${playlist.value.id}`
        )
      await store.replaceQueueAndPlay(tracks)
    }

    const addToQueue = async () => {
      if (!playlist.value?.tracks?.length) return
      const tracks = playlist.value.tracks.map(track => 
        `os:${track.id}:p:${playlist.value.id}`
      )
      await store.addToQueue(tracks)
    }

    const playSong = async (track) => {
      const url = `os:${track.id}:p:${playlist.value.id}`
      await store.addToQueueAndPlay(url)
    }

    const openTrackDetails = (track) => {
      // Convert OpenSubsonic track format to standard track format for modal
      const convertedTrack = {
        ...track,
        // Generate correct URI format for queue operations: os:trackId:p:playlistId
        uri: `os:${track.id}:p:${playlist.value.id}`,
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

    onMounted(async () => {
      console.log('PagePlaylistTracksOpensubsonic mounted')
      console.log('Route params:', route.params)
      console.log('Store connected:', store.isConnected)
      console.log('Store loading:', store.loading)
      
      // Initialize store if not already done
      if (!store.isConnected && !store.loading) {
        console.log('Initializing store...')
        await store.initialize()
        console.log('Store initialized, connected:', store.isConnected)
      }
      
      // Load playlist data if connected
      if (store.isConnected) {
        console.log('Loading playlist with ID:', route.params.id)
        await store.loadPlaylist(route.params.id)
        console.log('Playlist loaded:', store.currentPlaylist)
      } else {
        console.log('Store not connected, cannot load playlist')
      }
    })

    return {
      playlist,
      loading,
      error,
      selectedTrack,
      showTrackDetailsModal,
      formatTrackInfo,
      formatDuration,
      formatTotalDuration,
      formatDate,
      getCoverArtUrl,
      playPlaylist,
      shufflePlaylist,
      addToQueue,
      playSong,
      openTrackDetails,
      store,
      route
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

.playlist-info {
  color: #666;
}

.playlist-comment {
  white-space: pre-line;
}
</style>