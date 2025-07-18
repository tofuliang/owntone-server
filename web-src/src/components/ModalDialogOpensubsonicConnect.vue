<template>
  <ModalDialog
    :active="active"
    @close="$emit('close')"
    :title="$t('page.opensubsonic.connect.title')"
  >
    <div v-if="error" class="notification is-danger">
      {{ error }}
    </div>

    <div class="field">
      <label class="label">{{ $t('page.opensubsonic.connect.server_url') }}</label>
      <div class="control">
        <input
          class="input"
          type="text"
          v-model="form.server_url"
          :placeholder="$t('page.opensubsonic.connect.server_url_placeholder')"
        />
      </div>
    </div>

    <div class="field">
      <label class="label">{{ $t('page.opensubsonic.connect.username') }}</label>
      <div class="control">
        <input
          class="input"
          type="text"
          v-model="form.username"
          :placeholder="$t('page.opensubsonic.connect.username_placeholder')"
        />
      </div>
    </div>

    <div class="field">
      <label class="label">{{ $t('page.opensubsonic.connect.password') }}</label>
      <div class="control">
        <input
          class="input"
          type="password"
          v-model="form.password"
          :placeholder="$t('page.opensubsonic.connect.password_placeholder')"
        />
      </div>
    </div>

    <template #footer>
      <button
        class="button is-primary"
        :class="{ 'is-loading': loading }"
        @click="connect"
        :disabled="!isValid"
      >
        {{ $t('page.opensubsonic.connect.button') }}
      </button>
    </template>
  </ModalDialog>
</template>

<script>
import { ref, computed } from 'vue'
import { useOpensubsonicStore } from '@/stores/opensubsonic'
import ModalDialog from '@/components/ModalDialog.vue'

export default {
  name: 'ModalDialogOpensubsonicConnect',

  components: {
    ModalDialog
  },

  props: {
    active: {
      type: Boolean,
      required: true
    }
  },

  emits: ['close', 'connected'],

  setup(props, { emit }) {
    const store = useOpensubsonicStore()
    const error = computed(() => store.error)
    const form = ref({
      server_url: '',
      username: '',
      password: ''
    })

    const loading = computed(() => store.loading)
    const isValid = computed(() => {
      return (
        form.value.server_url &&
        form.value.username &&
        form.value.password
      )
    })

    const connect = async () => {
      store.clearError() // Clear any previous errors
      // Since backend auto-connects, we just need to update the UI
      form.value = { server_url: '', username: '', password: '' }
      emit('connected')
      emit('close')
    }

    return {
      form,
      loading,
      error,
      isValid,
      connect
    }
  }
}
</script>