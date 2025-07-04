<template>
  <div v-if="!isItem" class="py-5">
    <div class="media-content">
      <span
        :id="`index_${index}`"
        class="tag is-small has-text-weight-bold"
        v-text="index"
      />
    </div>
  </div>
  <div
    v-else
    class="media is-align-items-center mb-0"
    :class="{ 'is-clickable': isPlayable, 'is-not-allowed': !isPlayable }"
    @click="open"
  >
    <mdicon v-if="icon" class="media-left icon" :name="icon" />
    <control-image
      v-if="resolvedImageUrl"
      :url="resolvedImageUrl"
      :caption="lines && lines.length > 0 ? lines[0] : 'Artwork'"
      class="media-left is-small"
    />
    <control-image
      v-else-if="image && !artworkError"
      :url="image.url"
      :caption="image.caption"
      class="media-left is-small"
    />
    <!-- Optional: Placeholder for artworkError -->
    <div v-else-if="itemSource === 'opensubsonic' && coverArtId && artworkError" class="media-left is-small">
      <mdicon name="image-off" class="icon has-text-grey-light" title="Artwork not available" />
    </div>
    <div class="media-content">
      <div
        v-for="(line, position) in lines"
        :key="position"
        :class="{
          'is-size-6': position === 0,
          'is-size-7': position !== 0,
          'has-text-weight-bold': position !== 2,
          'has-text-grey': (position !== 0 || isRead) && isPlayable,
          'has-text-grey-light': !isPlayable
        }"
        v-text="line"
      />
      <div v-if="!isPlayable" class="is-size-7 has-text-grey">
        <slot name="reason" />
      </div>
    </div>
    <div v-if="progress" class="media-right">
      <control-progress :value="progress" />
    </div>
    <div class="media-right">
      <a @click.prevent.stop="openDetails">
        <mdicon class="icon has-text-grey" name="dots-vertical" size="16" />
      </a>
    </div>
  </div>
</template>

<script>
import ControlImage from '@/components/ControlImage.vue'
import ControlProgress from '@/components/ControlProgress.vue'
import services from '@/api/services' // Added for OpenSubsonic artwork

export default {
  name: 'ListItem',
  components: { ControlImage, ControlProgress },
  props: {
    icon: { default: null, type: String },
    image: { default: null, type: Object }, // Existing direct image prop
    coverArtId: { default: null, type: String }, // For ID-based artwork fetching
    itemSource: { default: 'local', type: String }, // To identify source e.g. 'opensubsonic'
    index: { default: null, type: [String, Number] },
    isItem: { default: true, type: Boolean },
    isPlayable: { default: true, type: Boolean },
    isRead: { default: false, type: Boolean },
    lines: { default: null, type: Array },
    progress: { default: null, type: Number }
  },
  emits: ['open', 'openDetails'],
  data() {
    return {
      resolvedImageUrl: null,
      artworkError: false
    }
  },
  watch: {
    coverArtId: {
      immediate: true,
      handler(newId) {
        this.fetchArtwork(newId)
      }
    },
    itemSource: {
      immediate: true,
      handler() {
        this.fetchArtwork(this.coverArtId)
      }
    }
  },
  methods: {
    open() {
      this.$emit('open')
    },
    openDetails() {
      this.$emit('openDetails')
    },
    async fetchArtwork(id) {
      if (this.itemSource === 'opensubsonic' && id) {
        this.artworkError = false
        this.resolvedImageUrl = null // Clear previous image
        try {
          // Assuming a default size, e.g., 100px. This could be a prop too.
          const response = await services.openSubsonicGetCoverArtConcreteUrl(id, 100)
          if (response && response.url) {
            this.resolvedImageUrl = response.url
          } else {
            this.artworkError = true
          }
        } catch (error) {
          console.error(`Error fetching OpenSubsonic artwork for ID ${id}:`, error)
          this.artworkError = true
          this.resolvedImageUrl = null
        }
      } else {
        this.resolvedImageUrl = null // Clear if not applicable
        this.artworkError = false
      }
    }
  }
}
</script>

<style scoped>
.is-not-allowed {
  cursor: not-allowed;
}
</style>
