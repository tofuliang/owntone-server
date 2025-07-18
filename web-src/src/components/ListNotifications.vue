<template>
  <section v-if="!notificationsStore.isEmpty" class="notifications">
    <div class="columns is-centered">
      <div class="column is-half">
        <div
          v-for="(notification, index) in notificationsStore.list"
          :key="index"
          class="notification"
          :class="notification.type ? `is-${notification.type}` : ''"
        >
          <button
            class="delete"
            @click="notificationsStore.remove(notification)"
          />
          <div class="text" v-text="notification.text" />
        </div>
      </div>
    </div>
  </section>
</template>

<script>
import { useNotificationsStore } from '@/stores/notifications'

export default {
  name: 'ListNotifications',
  setup() {
    return { notificationsStore: useNotificationsStore() }
  }
}
</script>

<style scoped>
.notifications {
  position: fixed;
  bottom: 4rem;
  z-index: 20000;
  width: 100%;
  pointer-events: none; /* Allow clicks to pass through container */
}

.notifications .notification {
  box-shadow:
    0 4px 8px 0 rgba(0, 0, 0, 0.2),
    0 6px 20px 0 rgba(0, 0, 0, 0.19);
  pointer-events: auto; /* Re-enable clicks on notifications */
  margin-bottom: 0.75rem;
  border-radius: 8px;
  animation: slideInUp 0.3s ease-out;
}

.notification .text {
  overflow-wrap: break-word;
  word-break: break-word;
  hyphens: auto;
  line-height: 1.4;
  max-height: none; /* Remove height restriction */
  overflow: visible; /* Remove scrollbar */
}

/* Error notifications specific styling */
.notification.is-danger {
  background-color: #f14668;
  color: white;
  border-left: 4px solid #dc143c;
}

.notification.is-danger .delete {
  background-color: rgba(255, 255, 255, 0.2);
  border-radius: 50%;
}

.notification.is-danger .delete:hover {
  background-color: rgba(255, 255, 255, 0.3);
}

/* Long text handling */
.notification .text {
  display: -webkit-box;
  -webkit-line-clamp: 6; /* Show max 6 lines */
  -webkit-box-orient: vertical;
  overflow: hidden;
  position: relative;
}

/* Fade out effect for long text */
.notification .text::after {
  content: '';
  position: absolute;
  bottom: 0;
  right: 0;
  width: 30%;
  height: 1.4em;
  background: linear-gradient(to right, transparent, currentColor);
  opacity: 0;
  transition: opacity 0.2s;
}

.notification .text:hover::after {
  opacity: 0.1;
}

/* Expand on hover for long messages */
.notification:hover .text {
  -webkit-line-clamp: unset;
  max-height: 20rem;
  overflow-y: auto;
  scrollbar-width: thin;
  scrollbar-color: rgba(255, 255, 255, 0.3) transparent;
}

.notification:hover .text::-webkit-scrollbar {
  width: 4px;
}

.notification:hover .text::-webkit-scrollbar-track {
  background: transparent;
}

.notification:hover .text::-webkit-scrollbar-thumb {
  background-color: rgba(255, 255, 255, 0.3);
  border-radius: 2px;
}

/* Slide in animation */
@keyframes slideInUp {
  from {
    transform: translateY(100%);
    opacity: 0;
  }
  to {
    transform: translateY(0);
    opacity: 1;
  }
}

/* Responsive design */
@media (max-width: 768px) {
  .notifications {
    bottom: 2rem;
  }
  
  .notifications .column {
    padding: 0 1rem;
  }
  
  .notification .text {
    font-size: 0.9rem;
    -webkit-line-clamp: 4; /* Fewer lines on mobile */
  }
}
</style>
