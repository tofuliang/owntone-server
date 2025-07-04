<template>
  <content-with-heading>
    <template #heading>
      <pane-title :content="{ title: $t('navigation.opensubsonic') }" />
      <!-- TODO: Add tabs for Playlists, Artists, Albums if desired -->
    </template>
    <template #content>
      <div>
        <h2 class="title is-4">OpenSubsonic Playlists</h2>
        <list-playlists
          v-if="playlists.items.length > 0"
          :items="playlists"
          :get-items="getItems"
          @item-primary="viewPlaylist"
          @item-play="playPlaylist"
        />
        <p v-else-if="isLoading" v-text="$t('messages.loading')" />
        <p v-else v-text="$t('messages.no_playlists_found')" />

        <!-- TODO: Sections for Artists and Albums -->
      </div>
    </template>
  </content-with-heading>
</template>

<script>
import ContentWithHeading from '@/templates/ContentWithHeading.vue'
import ListPlaylists from '@/components/ListPlaylists.vue'
import PaneTitle from '@/components/PaneTitle.vue'
import services from '@/api/services'
import { useServicesStore } from '@/stores/services'
import { useQueueStore } from '@/stores/queue'
import { GroupedList } from '@/lib/GroupedList'

export default {
  name: 'PageMusicOpenSubsonic',
  components: { ContentWithHeading, ListPlaylists, PaneTitle },
  setup() {
    const servicesStore = useServicesStore()
    const queueStore = useQueueStore()
    return { servicesStore, queueStore }
  },
  data() {
    return {
      playlists: new GroupedList(),
      isLoading: false,
      // TODO: Add data properties for artists and albums
    }
  },
  computed: {
    // Example: Check if OpenSubsonic is configured and enabled
    isServiceActive() {
      return this.servicesStore.isOpenSubsonicEnabled && this.servicesStore.isOpenSubsonicConfigured
    }
  },
  async mounted() {
    if (this.isServiceActive) {
      await this.fetchPlaylists()
      // TODO: Fetch artists and albums
    } else if (this.servicesStore.isOpenSubsonicEnabled && !this.servicesStore.isOpenSubsonicConfigured) {
        // Optionally, redirect to settings or show a message if not configured
        this.$router.push({ name: 'settings-online-services' })
    }
  },
  methods: {
    getItems(items) {
      return items // For ListPlaylists compatibility
    },
    async fetchPlaylists() {
      this.isLoading = true
      try {
        const response = await services.openSubsonicGetPlaylists()
        const result = response['subsonic-response']?.playlists?.playlist || []
        this.playlists = new GroupedList(
          result.map((p) => ({
            id: `opensubsonic:playlist:${p.id}`, // Create a unique URI
            name: p.name,
            item_count: p.songCount,
            // artwork_url: p.coverArt ? services.openSubsonicGetCoverArtUrl(p.coverArt, 100) : null, // If ListPlaylists supports artwork
            uri: `opensubsonic:playlist:${p.id}`
          }))
        )
      } catch (error) {
        console.error('Error fetching OpenSubsonic playlists:', error)
        // Handle error, e.g., show notification
      }
      this.isLoading = false
    },
    viewPlaylist(playlist) {
      // Playlist ID for routing is the raw ID, not the URI
      const rawId = playlist.id.replace('opensubsonic:playlist:', '')
      this.$router.push({ name: 'opensubsonic-playlist', params: { id: rawId } })
    },
    playPlaylist(playlist) {
      this.queueStore.addItems({ uris: [playlist.uri], options: { play: true, replace: true } })
    }
    // TODO: Methods for fetching/viewing artists and albums
  }
}
</script>

<i18n>
{
  "en": {
    "navigation.opensubsonic": "OpenSubsonic",
    "messages.loading": "Loading...",
    "messages.no_playlists_found": "No playlists found on OpenSubsonic server."
  }
}
</i18n>
