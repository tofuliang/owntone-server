<template>
  <div 
    ref="containerRef" 
    class="virtual-list-container"
    :style="{ height: containerHeight + 'px', overflow: 'auto' }"
    @scroll="handleScroll"
  >
    <div 
      class="virtual-list-spacer" 
      :style="{ height: totalHeight + 'px', position: 'relative' }"
    >
      <div 
        class="virtual-list-content"
        :style="{ 
          transform: `translateY(${offsetY}px)`,
          position: 'absolute',
          top: 0,
          left: 0,
          right: 0
        }"
      >
        <slot 
          v-for="(item, index) in visibleItems" 
          :key="getItemKey(item, startIndex + index)"
          :item="item" 
          :index="startIndex + index"
        />
      </div>
    </div>
  </div>
</template>

<script>
import { ref, computed, onMounted, onUnmounted, watch } from 'vue'

export default {
  name: 'VirtualList',
  
  props: {
    items: {
      type: Array,
      required: true
    },
    itemHeight: {
      type: Number,
      default: 60
    },
    containerHeight: {
      type: Number,
      default: 400
    },
    keyField: {
      type: String,
      default: 'id'
    },
    overscan: {
      type: Number,
      default: 5
    }
  },

  setup(props) {
    const containerRef = ref(null)
    const scrollTop = ref(0)

    const totalHeight = computed(() => props.items.length * props.itemHeight)
    
    const visibleCount = computed(() => Math.ceil(props.containerHeight / props.itemHeight))
    
    const startIndex = computed(() => {
      const index = Math.floor(scrollTop.value / props.itemHeight) - props.overscan
      return Math.max(0, index)
    })
    
    const endIndex = computed(() => {
      const index = startIndex.value + visibleCount.value + props.overscan * 2
      return Math.min(props.items.length - 1, index)
    })
    
    const visibleItems = computed(() => {
      return props.items.slice(startIndex.value, endIndex.value + 1)
    })
    
    const offsetY = computed(() => startIndex.value * props.itemHeight)

    const handleScroll = (event) => {
      scrollTop.value = event.target.scrollTop
    }

    const getItemKey = (item, index) => {
      return item[props.keyField] || index
    }

    // 当items变化时，重置滚动位置
    watch(() => props.items, () => {
      scrollTop.value = 0
      if (containerRef.value) {
        containerRef.value.scrollTop = 0
      }
    })

    return {
      containerRef,
      totalHeight,
      offsetY,
      visibleItems,
      startIndex,
      handleScroll,
      getItemKey
    }
  }
}
</script>

<style scoped>
.virtual-list-container {
  position: relative;
}

.virtual-list-spacer {
  width: 100%;
}

.virtual-list-content {
  width: 100%;
}
</style>