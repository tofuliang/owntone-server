<template>
  <tabs-settings />
  <content-with-heading>
    <template #heading>
      <pane-title :content="{ title: $t('settings.services.spotify.title') }" />
    </template>
    <template #content>
      <div v-if="servicesStore.isSpotifyEnabled">
        <div v-text="$t('settings.services.spotify.grant-access')" />
        <div
          class="notification help"
          v-text="
            $t('settings.services.spotify.requirements', {
              scopes: servicesStore.requiredSpotifyScopes.join(', ')
            })
          "
        />
        <div v-if="servicesStore.isSpotifyActive">
          <div
            v-text="
              $t('settings.services.spotify.user', {
                user: servicesStore.spotify.webapi_user
              })
            "
          />
          <div
            v-if="servicesStore.hasMissingSpotifyScopes"
            class="notification help is-danger is-light"
            v-text="
              $t('settings.services.spotify.reauthorize', {
                scopes: servicesStore.missingSpotifyScopes.join(', ')
              })
            "
          />
        </div>
        <div class="field is-grouped mt-5">
          <div v-if="servicesStore.isAuthorizationRequired" class="control">
            <a
              class="button"
              :href="servicesStore.spotify.oauth_uri"
              v-text="$t('settings.services.spotify.authorize')"
            />
          </div>
          <div v-if="servicesStore.isSpotifyActive" class="control">
            <button
              class="button is-danger"
              @click="logoutSpotify"
              v-text="$t('actions.logout')"
            />
          </div>
        </div>
      </div>
      <div v-else v-text="$t('settings.services.spotify.no-support')" />
    </template>
  </content-with-heading>
  <content-with-heading>
    <template #heading>
      <pane-title :content="{ title: $t('settings.services.lastfm.title') }" />
    </template>
    <template #content>
      <div v-if="servicesStore.isLastfmEnabled">
        <div v-text="$t('settings.services.lastfm.grant-access')" />
        <div
          class="notification help"
          v-text="$t('settings.services.lastfm.info')"
        />
        <div v-if="!servicesStore.isLastfmActive">
          <form @submit.prevent="loginLastfm">
            <div class="field is-grouped">
              <div class="control">
                <input
                  v-model="lastfmCredentials.user"
                  class="input"
                  type="text"
                  :placeholder="$t('settings.services.username')"
                />
                <div
                  v-if="lastfmErrors"
                  class="help is-danger"
                  v-text="lastfmErrors.user"
                />
              </div>
              <div class="control">
                <input
                  v-model="lastfmCredentials.password"
                  class="input"
                  type="password"
                  :placeholder="$t('settings.services.password')"
                />
                <div
                  v-if="lastfmErrors"
                  class="help is-danger"
                  v-text="lastfmErrors.password"
                />
              </div>
              <div class="control">
                <button
                  class="button"
                  type="submit"
                  v-text="$t('actions.login')"
                />
              </div>
            </div>
            <div
              v-if="lastfmErrors"
              class="help is-danger"
              v-text="lastfmErrors.error"
            />
          </form>
        </div>
        <div v-else>
          <button
            class="button is-danger"
            @click="logoutLastfm"
            v-text="$t('actions.logout')"
          />
        </div>
      </div>
      <div v-else v-text="$t('settings.services.lastfm.no-support')" />
    </template>
  </content-with-heading>
  <content-with-heading>
    <template #heading>
      <pane-title
        :content="{ title: $t('settings.services.opensubsonic.title') }"
      />
    </template>
    <template #content>
      <div v-if="servicesStore.isOpenSubsonicEnabled">
        <div
          class="notification help"
          v-text="$t('settings.services.opensubsonic.info')"
        />
        <form @submit.prevent="saveOpenSubsonicConfig">
          <div class="field">
            <label for="os-url" class="label">{{
              $t('settings.services.opensubsonic.server_url')
            }}</label>
            <div class="control">
              <input
                id="os-url"
                v-model="opensubsonicConfig.server_url"
                class="input"
                type="text"
                :placeholder="
                  $t('settings.services.opensubsonic.server_url_placeholder')
                "
              />
            </div>
            <p
              v-if="opensubsonicErrors.server_url"
              class="help is-danger"
              v-text="opensubsonicErrors.server_url"
            />
          </div>

          <div class="field">
            <label for="os-user" class="label">{{
              $t('settings.services.opensubsonic.username')
            }}</label>
            <div class="control">
              <input
                id="os-user"
                v-model="opensubsonicConfig.username"
                class="input"
                type="text"
                :placeholder="$t('settings.services.username')"
              />
            </div>
            <p
              v-if="opensubsonicErrors.username"
              class="help is-danger"
              v-text="opensubsonicErrors.username"
            />
          </div>

          <div class="field">
            <label for="os-pass" class="label">{{
              $t('settings.services.opensubsonic.password')
            }}</label>
            <div class="control">
              <input
                id="os-pass"
                v-model="opensubsonicConfig.password"
                class="input"
                type="password"
                :placeholder="$t('settings.services.password')"
              />
            </div>
            <p
              v-if="opensubsonicErrors.password"
              class="help is-danger"
              v-text="opensubsonicErrors.password"
            />
            <p
              class="help"
              v-text="$t('settings.services.opensubsonic.password_help')"
            />
          </div>
          <div
            v-if="opensubsonicMessage"
            class="notification is-info is-light my-3 py-2"
            v-text="opensubsonicMessage"
          />
          <div
            v-if="opensubsonicErrors.error"
            class="notification is-danger is-light my-3 py-2"
            v-text="opensubsonicErrors.error"
          />

          <div class="field is-grouped">
            <div class="control">
              <button
                class="button is-primary"
                type="submit"
                v-text="$t('actions.save')"
              />
            </div>
            <div class="control">
              <button
                class="button"
                type="button"
                @click="testOpenSubsonicConnection"
                v-text="$t('settings.services.opensubsonic.test_connection')"
              />
            </div>
          </div>
        </form>
      </div>
      <div v-else v-text="$t('settings.services.opensubsonic.no-support')" />
    </template>
  </content-with-heading>
</template>

<script>
import ContentWithHeading from '@/templates/ContentWithHeading.vue'
import PaneTitle from '@/components/PaneTitle.vue'
import TabsSettings from '@/components/TabsSettings.vue'
import services from '@/api/services'
import { useServicesStore } from '@/stores/services'

export default {
  name: 'PageSettingsOnlineServices',
  components: { ContentWithHeading, PaneTitle, TabsSettings },
  setup() {
    return { servicesStore: useServicesStore() }
  },
  data() {
    return {
      lastfmCredentials: { password: '', user: '' },
      lastfmErrors: { error: '', password: '', user: '' },
      opensubsonicConfig: {
        server_url: '',
        username: '',
        password: ''
        // enabled: can be part of this if we allow toggling from UI,
        // otherwise it's just read from servicesStore.isOpenSubsonicEnabled
      },
      opensubsonicErrors: { error: '', server_url: '', username: '', password: '' },
      opensubsonicMessage: '' // For success/testing messages
    }
  },
  methods: {
    loginLastfm() {
      services.loginLastfm(this.lastfmCredentials).then((data) => {
        this.lastfmErrors = data.errors
        this.lastfmCredentials.password = ''
        if (data.success) {
          this.lastfmCredentials.user = ''
        }
      })
    },
    logoutLastfm() {
      services.logoutLastfm()
    },
    logoutSpotify() {
      services.logoutSpotify()
    },
    async saveOpenSubsonicConfig() {
      this.opensubsonicErrors = {} // Clear previous errors
      try {
        const response = await services.opensubsonic.saveConfig(
          this.opensubsonicConfig
        )
        if (response.success) {
          // Optionally show a success message
          this.servicesStore.initialiseOpenSubsonic() // Refresh state
          this.opensubsonicMessage = this.$t(
            'settings.services.opensubsonic.save_success'
          )
          setTimeout(() => (this.opensubsonicMessage = ''), 3000)
        } else {
          this.opensubsonicErrors = response.errors || {
            error: this.$t('settings.services.opensubsonic.save_failed')
          }
        }
      } catch (error) {
        this.opensubsonicErrors = {
          error:
            error.message ||
            this.$t('settings.services.opensubsonic.save_failed')
        }
        console.error('Error saving OpenSubsonic config:', error)
      }
    },
    async testOpenSubsonicConnection() {
      this.opensubsonicErrors = {}
      this.opensubsonicMessage = this.$t(
        'settings.services.opensubsonic.testing_connection'
      )
      try {
        const response = await services.opensubsonic.testConnection(
          this.opensubsonicConfig // Send current form data for test
        )
        if (response.success) {
          this.opensubsonicMessage = this.$t(
            'settings.services.opensubsonic.test_success'
          )
        } else {
          this.opensubsonicErrors = {
            error:
              response.message ||
              this.$t('settings.services.opensubsonic.test_failed')
          }
          this.opensubsonicMessage = ''
        }
      } catch (error) {
        this.opensubsonicErrors = {
          error:
            error.message ||
            this.$t('settings.services.opensubsonic.test_failed')
        }
        this.opensubsonicMessage = ''
        console.error('Error testing OpenSubsonic connection:', error)
      }
      setTimeout(() => {
        if (
          this.opensubsonicMessage ===
          this.$t('settings.services.opensubsonic.testing_connection') ||
          this.opensubsonicMessage ===
            this.$t('settings.services.opensubsonic.test_success')
        ) {
          this.opensubsonicMessage = ''
        }
      }, 5000)
    }
  },
  created() {
    // Initialize local form data from store when component is created
    this.opensubsonicConfig.server_url = this.servicesStore.openSubsonicServerUrl || ''
    this.opensubsonicConfig.username = this.servicesStore.openSubsonicUsername || ''
    // Password is not pre-filled for security
  },
  watch: {
    // Keep local form data in sync if store changes (e.g. after initial load)
    'servicesStore.openSubsonicServerUrl'(newVal) {
      this.opensubsonicConfig.server_url = newVal || ''
    },
    'servicesStore.openSubsonicUsername'(newVal) {
      this.opensubsonicConfig.username = newVal || ''
    }
  }
}
</script>
