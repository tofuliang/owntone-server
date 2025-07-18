<template>
  <ContentWithHeading>
    <template #heading>
      <div class="level">
        <div class="level-left">
          <div class="level-item">
            <h1 class="title">{{ $t('page.music.tabs.opensubsonic') }}</h1>
          </div>
        </div>
        <div class="level-right">
          <!-- Removed connect/disconnect buttons since backend auto-connects -->
        </div>
      </div>
    </template>

    <div v-if="loading" class="content has-text-centered">
      <p>{{ $t('page.opensubsonic.status.loading') }}</p>
    </div>

    <div v-else-if="!isConnected" class="content has-text-centered">
      <p>{{ $t('page.opensubsonic.status.not_available') }}</p>
      <p v-if="error" class="has-text-danger">
        {{ $t('page.opensubsonic.status.error', { error }) }}
      </p>
    </div>

    <div v-else>
      <div class="notification is-success is-light">
        {{ $t('page.opensubsonic.status.connected', { server: serverUrl }) }}
      </div>
      <div class="level">
        <div class="level-left">
          <div class="level-item">
            <div class="tabs">
              <ul>
                <li :class="{ 'is-active': currentTab === 'artists' }">
                  <a @click="currentTab = 'artists'">{{
                    $t('page.opensubsonic.tabs.artists')
                  }}</a>
                </li>
                <li :class="{ 'is-active': currentTab === 'albums' }">
                  <a @click="currentTab = 'albums'">{{
                    $t('page.opensubsonic.tabs.albums')
                  }}</a>
                </li>
                <li :class="{ 'is-active': currentTab === 'playlists' }">
                  <a @click="currentTab = 'playlists'">{{
                    $t('page.opensubsonic.tabs.playlists')
                  }}</a>
                </li>
                <li :class="{ 'is-active': currentTab === 'search' }">
                  <a @click="currentTab = 'search'">{{
                    $t('page.opensubsonic.tabs.search')
                  }}</a>
                </li>
              </ul>
            </div>
          </div>
        </div>
      </div>

      <div class="tab-content">
        <div
          v-if="
            (artistsLoading && currentTab === 'artists') ||
            (albumsLoading && currentTab === 'albums') ||
            (playlistsLoading && currentTab === 'playlists')
          "
          class="has-text-centered"
        >
          <LoaderListItem />
        </div>
        <template v-else>
          <ListArtistsOpensubsonic v-if="currentTab === 'artists'" />
          <ListAlbumsOpensubsonic v-if="currentTab === 'albums'" />
          <ListPlaylistsOpensubsonic v-if="currentTab === 'playlists'" />
          <SearchOpensubsonic v-if="currentTab === 'search'" />
        </template>
      </div>
    </div>
  </ContentWithHeading>
</template>

<script>
import { computed, onMounted, watch } from 'vue'
import { useOpensubsonicStore } from '@/stores/opensubsonic'
import ContentWithHeading from '@/templates/ContentWithHeading.vue'
import ListArtistsOpensubsonic from '@/components/ListArtistsOpensubsonic.vue'
import SearchOpensubsonic from '@/components/SearchOpensubsonic.vue'
import ListAlbumsOpensubsonic from '@/components/ListAlbumsOpensubsonic.vue'
import ListPlaylistsOpensubsonic from '@/components/ListPlaylistsOpensubsonic.vue'
import LoaderListItem from '@/components/LoaderListItem.vue'

export default {
  name: 'PageMusicOpensubsonic',

  components: {
    ContentWithHeading,
    ListArtistsOpensubsonic,
    ListAlbumsOpensubsonic,
    ListPlaylistsOpensubsonic,
    SearchOpensubsonic,
    LoaderListItem
  },

  setup() {
    const store = useOpensubsonicStore()

    const isConnected = computed(() => store.isConnected)
    const error = computed(() => store.error)
    const loading = computed(() => store.loading)
    const serverUrl = computed(() => store.serverUrl)
    const artistsLoading = computed(() => store.artistsLoading)
    const albumsLoading = computed(() => store.albumsLoading)
    const playlistsLoading = computed(() => store.playlistsLoading)
    const currentTab = computed({
      get: () => store.currentTab,
      set: (value) => store.setCurrentTab(value)
    })

    // Initialize OpenSubsonic status when component mounts
    onMounted(async () => {
      await store.initialize()

      // Check if we should switch to a specific tab based on hash
      if (window.location.hash === '#albums') {
        store.setCurrentTab('albums')
      } else if (window.location.hash === '#artists') {
        store.setCurrentTab('artists')
      } else if (window.location.hash === '#playlists') {
        store.setCurrentTab('playlists')
      }
    })

    // 监听 isConnected，连接成功时加载数据
    watch(
      isConnected,
      async (val) => {
        if (val) {
          await Promise.all([
            store.loadArtists(),
            store.loadAlbums(),
            store.loadPlaylists()
          ])
        }
      },
      { immediate: true }
    )

    return {
      currentTab,
      isConnected,
      error,
      loading,
      serverUrl,
      artistsLoading,
      albumsLoading,
      playlistsLoading,
      store
    }
  }
}
</script>
