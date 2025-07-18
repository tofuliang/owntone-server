<template>
  <div class="opensubsonic-playlists">
    <div v-if="loading" class="has-text-centered">
      <LoaderListItem />
    </div>
    <div v-else-if="error" class="notification is-danger">
      {{ error }}
    </div>
    <div v-else>
      <div v-if="playlists.length === 0" class="content has-text-centered">
        <p>{{ $t('page.opensubsonic.playlists.empty') }}</p>
      </div>
      <div v-else class="list">
        <ListItem
          v-for="playlist in playlists"
          :key="playlist.id"
          :lines="[
            playlist.name,
            formatPlaylistInfo(playlist)
          ]"
          :image="playlist.artwork_url ? { url: playlist.artwork_url, caption: playlist.name } : null"
          @open="showPlaylist(playlist)"
          @openDetails="showDetails(playlist, 'playlist')"
        />
      </div>
    </div>

    <!-- Playlist Details Modal -->
    <modal-dialog-playable
      v-if="selectedItem"
      :buttons="modalButtons"
      :item="modalItem"
      :show="showDetailsModal"
      @close="showDetailsModal = false"
    />
  </div>
</template>

<script>
import { computed, onMounted, ref } from 'vue'
import { useRouter } from 'vue-router'
import { useI18n } from 'vue-i18n'
import { useOpensubsonicStore } from '@/stores/opensubsonic'
import ListItem from '@/components/ListItem.vue'
import LoaderListItem from '@/components/LoaderListItem.vue'
import ModalDialogPlayable from '@/components/ModalDialogPlayable.vue'

export default {
  name: 'ListPlaylistsOpensubsonic',

  components: {
    ListItem,
    LoaderListItem,
    ModalDialogPlayable
  },

  setup() {
    const store = useOpensubsonicStore()
    const router = useRouter()
    const { t, tc } = useI18n()

    // Modal dialog state
    const selectedItem = ref(null)
    const showDetailsModal = ref(false)

    const playlists = computed(() => store.playlists)
    const loading = computed(() => store.playlistsLoading)
    const error = computed(() => store.error)

    const formatPlaylistInfo = (playlist) => {
      const parts = []
      if (playlist.songCount) {
        parts.push(`${playlist.songCount} tracks`)
      }
      if (playlist.owner) {
        parts.push(`by ${playlist.owner}`)
      }
      return parts.join(' • ')
    }

    const getCoverArtUrl = (id) => {
      // TODO: Implement cover art URL generation
      return null
    }

    const showPlaylist = (playlist) => {
      router.push({
        name: 'playlist-opensubsonic',
        params: { id: playlist.id }
      })
    }

    // Modal dialog computed properties
    const modalButtons = computed(() => {
      return []
    })

    const modalItem = computed(() => {
      if (!selectedItem.value) return null

      const item = selectedItem.value
      
      if (item.type === 'playlist') {
        return {
          name: item.name,
          properties: [
            { key: 'property.tracks', value: item.songCount ? `${item.songCount} tracks` : null },
            { key: 'property.owner', value: item.owner }
          ],
          uri: `os:playlist:${item.id}`
        }
      }
      
      return null
    })

    const showDetails = (item, type) => {
      selectedItem.value = { ...item, type }
      showDetailsModal.value = true
    }

    onMounted(async () => {
      if (store.isConnected && playlists.value.length === 0) {
        await store.loadPlaylists()
      }
    })

    return {
      playlists,
      loading,
      error,
      formatPlaylistInfo,
      getCoverArtUrl,
      showPlaylist,
      selectedItem,
      showDetailsModal,
      modalButtons,
      modalItem,
      showDetails
    }
  }
}
</script>