<template>
  <div class="opensubsonic-artists">
    <div v-if="loading && artists.length === 0" class="has-text-centered">
      <LoaderListItem />
    </div>
    <div v-else-if="error" class="notification is-danger">
      {{ error }}
    </div>
    <div v-else>
      <div v-if="artists.length === 0" class="content has-text-centered">
        <p>{{ $t('page.opensubsonic.artists.empty') }}</p>
      </div>
      <div v-else>
        <!-- 如果艺人数量较少，使用普通列表 -->
        <div v-if="artists.length <= 100" class="list">
          <ListItem
            v-for="artist in artists"
            :key="artist.id"
            :lines="[artist.name, artist.albumCount ? `${artist.albumCount} albums` : '']"
            :image="getArtistImage(artist)"
            @open="showArtist(artist)"
            @openDetails="showDetails(artist, 'artist')"
          />
        </div>
        
        <!-- 如果艺人数量较多，使用虚拟列表 -->
        <div v-else>
          <p class="has-text-grey has-text-centered" style="margin-bottom: 1rem;">
            Total: {{ artists.length }} artists
          </p>
          <VirtualList
            :items="artists"
            :item-height="60"
            :container-height="Math.min(600, windowHeight - 200)"
            key-field="id"
          >
            <template #default="{ item: artist }">
              <ListItem
                :lines="[artist.name, artist.albumCount ? `${artist.albumCount} albums` : '']"
                :image="getArtistImage(artist)"
                @open="showArtist(artist)"
                @openDetails="showDetails(artist, 'artist')"
              />
            </template>
          </VirtualList>
        </div>
      </div>
    </div>

    <!-- Artist Details Modal -->
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
import { computed, onMounted, onUnmounted, ref } from 'vue'
import { useRouter } from 'vue-router'
import { useI18n } from 'vue-i18n'
import { useOpensubsonicStore } from '@/stores/opensubsonic'
import ListItem from '@/components/ListItem.vue'
import LoaderListItem from '@/components/LoaderListItem.vue'
import ModalDialogPlayable from '@/components/ModalDialogPlayable.vue'
import VirtualList from '@/components/VirtualList.vue'

export default {
  name: 'ListArtistsOpensubsonic',

  components: {
    ListItem,
    LoaderListItem,
    ModalDialogPlayable,
    VirtualList
  },

  setup() {
    const store = useOpensubsonicStore()
    const router = useRouter()
    const { t } = useI18n()

    // Modal dialog state
    const selectedItem = ref(null)
    const showDetailsModal = ref(false)
    
    // Window height for virtual list
    const windowHeight = ref(600) // Default fallback

    const artists = computed(() => store.artists)
    const loading = computed(() => store.artistsLoading)
    const error = computed(() => store.error)

    const showArtist = (artist) => {
      router.push({
        name: 'music-opensubsonic-artist',
        params: { id: artist.id }
      })
    }

    // Modal dialog computed properties
    const modalButtons = computed(() => {
      return []
    })

    const modalItem = computed(() => {
      if (!selectedItem.value) return null

      const item = selectedItem.value
      
      if (item.type === 'artist') {
        return {
          name: item.name,
          properties: [
            { key: 'property.albums', value: t('page.opensubsonic.artists.album_count', { count: item.albumCount }) }
          ],
          uri: `os:artist:${item.id}`
        }
      }
      
      return null
    })

    const showDetails = (item, type) => {
      selectedItem.value = { ...item, type }
      showDetailsModal.value = true
    }

    const getArtistImage = (artist) => {
      if (artist.artwork_url) {
        return { url: artist.artwork_url, caption: artist.name }
      }
      return null
    }

    // Update window height safely
    const updateWindowHeight = () => {
      if (typeof window !== 'undefined') {
        windowHeight.value = window.innerHeight
      }
    }

    onMounted(async () => {
      // Set initial window height
      updateWindowHeight()
      
      // Add resize listener
      if (typeof window !== 'undefined') {
        window.addEventListener('resize', updateWindowHeight)
      }
      
      if (store.isConnected && artists.value.length === 0) {
        await store.loadArtists()
      }
    })

    onUnmounted(() => {
      // Clean up resize listener
      if (typeof window !== 'undefined') {
        window.removeEventListener('resize', updateWindowHeight)
      }
    })

    return {
      artists,
      loading,
      error,
      showArtist,
      selectedItem,
      showDetailsModal,
      modalButtons,
      modalItem,
      showDetails,
      getArtistImage,
      windowHeight
    }
  }
}
</script>