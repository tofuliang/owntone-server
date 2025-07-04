<template>
  <content-with-search
    :components="components"
    :expanded="expanded"
    :get-items="getItems"
    :history="history"
    :results="results"
    :opensubsonic-results="opensubsonicResults"
    :search-sources="searchSources"
    v-model:current-search-source="currentSearchSource"
    @search="search"
    @search-library="search"
    @search-query="openSearch"
    @search-spotify="searchSpotify"
    @search-opensubsonic="searchOpenSubsonicSource"
    @expand="expand"
  >
    <template #help>
      <i18n-t
        tag="p"
        class="help has-text-centered"
        keypath="page.search.help"
        scope="global"
      >
        <template #query>
          <code>query:</code>
        </template>
        <template #help>
          <a
            href="https://owntone.github.io/owntone-server/smart-playlists/"
            target="_blank"
            v-text="$t('page.search.expression')"
          />
        </template>
      </i18n-t>
    </template>

    <!-- Slot for OpenSubsonic results, to be handled by ContentWithSearch or displayed here -->
    <template #opensubsonicContent>
      <div v-if="currentSearchSource === 'opensubsonic' || isOpenSubsonicNonEmpty">
        <div v-if="opensubsonicResults.songs && opensubsonicResults.songs.items.length > 0" class="my-5">
          <h2 class="title is-4">OpenSubsonic Songs</h2>
          <list-tracks :items="opensubsonicResults.songs" :show-subtitles="true" />
        </div>
        <div v-if="opensubsonicResults.albums && opensubsonicResults.albums.items.length > 0" class="my-5">
          <h2 class="title is-4">OpenSubsonic Albums</h2>
          <list-albums :items="opensubsonicResults.albums" />
        </div>
        <div v-if="opensubsonicResults.artists && opensubsonicResults.artists.items.length > 0" class="my-5">
          <h2 class="title is-4">OpenSubsonic Artists</h2>
          <list-artists :items="opensubsonicResults.artists" />
        </div>
         <div v-if="!isOpenSubsonicNonEmpty && currentSearchSource === 'opensubsonic' && searchStore.query" class="my-5">
            <p class="has-text-centered">{{ $t('page.search.no-results') }} (OpenSubsonic)</p>
        </div>
      </div>
    </template>
  </content-with-search>
</template>

<script>
import ContentWithSearch from '@/templates/ContentWithSearch.vue'
import { GroupedList } from '@/lib/GroupedList'
import ListAlbums from '@/components/ListAlbums.vue'
import ListArtists from '@/components/ListArtists.vue'
import ListComposers from '@/components/ListComposers.vue'
import ListPlaylists from '@/components/ListPlaylists.vue'
import ListTracks from '@/components/ListTracks.vue'
import library from '@/api/library'
import services from '@/api/services'
import { useSearchStore } from '@/stores/search'
import { useServicesStore } from '@/stores/services'

const PAGE_SIZE = 3
const OS_PAGE_SIZE = 20
const OS_ALBUM_ARTIST_PAGE_SIZE = 5

const SEARCH_TYPES = [
  'track',
  'artist',
  'album',
  'composer',
  'playlist',
  'audiobook',
  'podcast'
]

export default {
  name: 'PageSearchLibrary',
  components: { ContentWithSearch },
  setup() {
    return {
      components: { // For local library results
        album: ListAlbums,
        audiobook: ListAlbums,
        artist: ListArtists,
        composer: ListComposers,
        playlist: ListPlaylists,
        podcast: ListAlbums,
        track: ListTracks
      },
      searchStore: useSearchStore(),
      servicesStore: useServicesStore()
    }
  },
  data() {
    return {
      limit: PAGE_SIZE,
      results: new Map(), // For local library results
      opensubsonicResults: {
        songs: new GroupedList(),
        albums: new GroupedList(),
        artists: new GroupedList()
      },
      types: SEARCH_TYPES, // For local library search types
      searchSources: ['library', 'opensubsonic', 'spotify'],
      currentSearchSource: 'library'
    }
  },
  computed: {
    expanded() {
      // This computed property might need adjustment if OpenSubsonic results
      // are displayed in a way that affects the "expanded" view concept.
      // For now, it's based on local library search types.
      return this.types.length === 1
    },
    history() {
      return this.searchStore.history
    },
    isOpenSubsonicNonEmpty() {
      return (
        (this.opensubsonicResults.songs && this.opensubsonicResults.songs.items.length > 0) ||
        (this.opensubsonicResults.albums && this.opensubsonicResults.albums.items.length > 0) ||
        (this.opensubsonicResults.artists && this.opensubsonicResults.artists.items.length > 0)
      )
    }
  },
  mounted() {
    // Load initial search source from store if needed, or default
    this.currentSearchSource = this.searchStore.source || 'library';
    if (this.searchStore.query) {
        this.search()
    }
  },
  methods: {
    expand(type) {
      // This method is for expanding a single category in local library results.
      // For OpenSubsonic, we fetch all types at once.
      if (this.currentSearchSource === 'library') {
        this.search([type], -1)
      }
    },
    getItems(items) {
      return items // Used by ContentWithSearch for local library results
    },
    openSearch(query) {
      this.searchStore.query = query
      this.search()
    },
    reset() {
      // Reset local library results
      this.results.clear()
      this.types.forEach((type) => {
        this.results.set(type, new GroupedList())
      })
      // Reset OpenSubsonic results
      this.opensubsonicResults.songs = new GroupedList()
      this.opensubsonicResults.albums = new GroupedList()
      this.opensubsonicResults.artists = new GroupedList()
    },
    search(types = SEARCH_TYPES, limit = PAGE_SIZE) {
      if (this.searchStore.query) {
        this.types = types
        this.limit = limit
        this.searchStore.query = this.searchStore.query.trim()

        this.reset()

        if (this.currentSearchSource === 'library') {
          this.types.forEach((type) => {
            this.searchItems(type)
          })
        }

        if (this.currentSearchSource === 'opensubsonic' && this.servicesStore.isOpenSubsonicEnabled) {
           this.searchOpenSubsonic(this.searchStore.query)
        }
        // Note: Spotify search is handled by navigating to a different page.
        // If an "all" source is added, logic here would need to call both searchItems and searchOpenSubsonic.

        this.searchStore.add(this.searchStore.query)
        this.searchStore.source = this.currentSearchSource // Persist current source
      } else {
        this.reset() // Clear results if query is empty
      }
    },
    searchItems(type) { // Local library search
      const music = type !== 'audiobook' && type !== 'podcast'
      const kind = (music && 'music') || type
      const parameters = {
        limit: this.limit,
        type: (music && type) || 'album'
      }
      if (this.searchStore.query.startsWith('query:')) {
        parameters.expression = `(${this.searchStore.query.replace(/^query:/u, '').trim()}) and media_kind is ${kind}`
      } else if (music) {
        parameters.query = this.searchStore.query
        parameters.media_kind = kind
      } else {
        parameters.expression = `(album includes "${this.searchStore.query}" or artist includes "${this.searchStore.query}") and media_kind is ${kind}`
      }
      library.search(parameters).then((data) => {
        this.results.set(type, new GroupedList(data[`${parameters.type}s`]))
      })
    },
    searchSpotify() {
      this.searchStore.source = 'spotify'; // Persist source before navigating
      this.$router.push({ name: 'search-spotify' })
    },
    searchOpenSubsonicSource() {
      this.currentSearchSource = 'opensubsonic'
      this.searchStore.source = 'opensubsonic'; // Update store
      this.search()
    },
    async searchOpenSubsonic(query) {
      if (!this.servicesStore.isOpenSubsonicEnabled || !query) {
        this.opensubsonicResults.songs = new GroupedList()
        this.opensubsonicResults.albums = new GroupedList()
        this.opensubsonicResults.artists = new GroupedList()
        return
      }
      try {
        const params = {
          query: query,
          songCount: OS_PAGE_SIZE,
          albumCount: OS_ALBUM_ARTIST_PAGE_SIZE,
          artistCount: OS_ALBUM_ARTIST_PAGE_SIZE
        }
        const response = await services.openSubsonicSearch(params)
        // Assuming backend forwards the "subsonic-response" object directly
        const result = response['subsonic-response']?.searchResult3 || {}

        this.opensubsonicResults.songs = new GroupedList(
          (result.song || []).map((s) => ({
            id: `opensubsonic:song:${s.id}`,
            name: s.title,
            title: s.title,
            artist: s.artist,
            album: s.album,
            duration_ms: s.duration ? s.duration * 1000 : 0,
            uri: `opensubsonic:song:${s.id}`,
            coverArtId: s.coverArt // Pass the ID for components to use
          }))
        )
        this.opensubsonicResults.albums = new GroupedList(
          (result.album || []).map((a) => ({
            id: `opensubsonic:album:${a.id}`,
            name: a.name,
            artist: a.artist,
            uri: `opensubsonic:album:${a.id}`,
            coverArtId: a.coverArt // Pass the ID
          }))
        )
        this.opensubsonicResults.artists = new GroupedList(
          (result.artist || []).map((ar) => ({
            id: `opensubsonic:artist:${ar.id}`,
            name: ar.name,
            uri: `opensubsonic:artist:${ar.id}`,
            coverArtId: ar.coverArt // Pass the ID
          }))
        )
      } catch (error) {
        console.error('Error searching OpenSubsonic:', error)
        this.opensubsonicResults.songs = new GroupedList()
        this.opensubsonicResults.albums = new GroupedList()
        this.opensubsonicResults.artists = new GroupedList()
      }
    }
  }
}
</script>
