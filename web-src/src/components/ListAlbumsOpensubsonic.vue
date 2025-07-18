<template>
  <div class="opensubsonic-albums">
    <div v-if="loading" class="has-text-centered">
      <LoaderListItem />
    </div>
    <div v-else-if="error" class="notification is-danger">
      {{ error }}
    </div>
    <div v-else>
      <div v-if="albums.length === 0" class="content has-text-centered">
        <p>{{ $t('page.opensubsonic.albums.empty') }}</p>
      </div>
      <div v-else class="list">
        <ListItem
          v-for="album in albums"
          :key="album.id"
          :lines="[
            album.name,
            album.artist ? `by ${album.artist}` : ''
          ]"
          :image="album.artwork_url ? { url: album.artwork_url, caption: album.name } : null"
          @open="showAlbum(album)"
          @openDetails="showDetails(album, 'album')"
        />
      </div>
    </div>

    <!-- Album Details Modal -->
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
  name: 'ListAlbumsOpensubsonic',

  components: {
    ListItem,
    LoaderListItem,
    ModalDialogPlayable
  },

  setup() {
    const store = useOpensubsonicStore()
    const router = useRouter()
    const { t } = useI18n()

    // Modal dialog state
    const selectedItem = ref(null)
    const showDetailsModal = ref(false)

    const albums = computed(() => store.albums)
    const loading = computed(() => store.albumsLoading)
    const error = computed(() => store.error)

    const getCoverArtUrl = (id) => {
      return store.getArtworkUrl(id)
    }

    const showAlbum = (album) => {
      router.push({
        name: 'music-opensubsonic-album',
        params: { id: album.id }
      })
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
            { key: 'property.artist', value: item.artist },
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

    onMounted(async () => {
      if (store.isConnected && albums.value.length === 0) {
        await store.loadAlbums()
      }
    })

    return {
      albums,
      loading,
      error,
      getCoverArtUrl,
      showAlbum,
      selectedItem,
      showDetailsModal,
      modalButtons,
      modalItem,
      showDetails
    }
  }
}
</script>