<template>
  <content-with-heading :loading="isLoading">
    <template #heading>
      <pane-title :content="{ title: playlistName, subtitle: $t('property.tracks') }" />
    </template>
    <template #content>
      <list-tracks
        v-if="tracks.items.length > 0"
        :items="tracks"
        :show-subtitles="true"
        @item-primary="playTrack"
        @item-add-queue="addTrackToQueue"
      />
      <p v-else-if="!isLoading" v-text="$t('messages.no_tracks_found')" />
    </template>
  </content-with-heading>
</template>

<script>
import ContentWithHeading from '@/templates/ContentWithHeading.vue'
import ListTracks from '@/components/ListTracks.vue'
import PaneTitle from '@/components/PaneTitle.vue'
import services from '@/api/services'
import { useQueueStore } from '@/stores/queue'
import { GroupedList } from '@/lib/GroupedList'

export default {
  name: 'PageOpenSubsonicPlaylistTracks',
  components: { ContentWithHeading, ListTracks, PaneTitle },
  props: {
    id: { type: String, required: true } // Playlist ID from router params
  },
  setup() {
    const queueStore = useQueueStore()
    return { queueStore }
  },
  data() {
    return {
      playlistName: '',
      tracks: new GroupedList(),
      isLoading: false
    }
  },
  async mounted() {
    await this.fetchPlaylistTracks()
  },
  methods: {
    async fetchPlaylistTracks() {
      this.isLoading = true
      try {
        const response = await services.openSubsonicGetPlaylist(this.id)
        const playlistData = response['subsonic-response']?.playlist || {}

        this.playlistName = playlistData.name || this.$t('property.playlist')

        const trackEntries = playlistData.entry || []
        this.tracks = new GroupedList(
          trackEntries.map((s) => ({
            id: `opensubsonic:song:${s.id}`,
            name: s.title,
            title: s.title,
            artist: s.artist,
            album: s.album,
            duration_ms: s.duration ? s.duration * 1000 : 0,
            uri: `opensubsonic:song:${s.id}`,
            // artwork_url: s.coverArt ? services.openSubsonicGetCoverArtUrl(s.coverArt, 100) : null,
            coverArt: s.coverArt
          }))
        )
      } catch (error) {
        console.error(`Error fetching tracks for OpenSubsonic playlist ${this.id}:`, error)
        // Handle error
      }
      this.isLoading = false
    },
    playTrack(track) {
      this.queueStore.addItems({ uris: [track.uri], options: { play: true, replace: false } }) // Or replace current track
    },
    addTrackToQueue(track) {
       this.queueStore.addItems({ uris: [track.uri], options: { play: false, replace: false } })
    }
  }
}
</script>

<i18n>
{
  "en": {
    "property.playlist": "Playlist",
    "property.tracks": "Tracks",
    "messages.no_tracks_found": "No tracks found in this playlist."
  }
}
</i18n>
