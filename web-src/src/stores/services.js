import { defineStore } from 'pinia'
import services from '@/api/services'

export const useServicesStore = defineStore('ServicesStore', {
  actions: {
    async initialiseLastfm() {
      this.lastfm = await services.lastfm()
    },
    initialiseSpotify() {
      services.spotify().then((data) => {
        this.spotify = data
        if (this.spotifyTimerId > 0) {
          clearTimeout(this.spotifyTimerId)
          this.spotifyTimerId = 0
        }
        if (data.webapi_token_expires_in > 0 && data.webapi_token) {
          this.spotifyTimerId = setTimeout(
            () => this.initialiseSpotify(),
            1000 * data.webapi_token_expires_in
          )
        }
      })
    },
    async initialiseOpenSubsonic() {
      try {
        this.opensubsonic = await services.opensubsonic.status()
      } catch (error) {
        console.error('Failed to initialize OpenSubsonic settings:', error)
        this.opensubsonic = { enabled: false, configured: false, error: true }
      }
    }
  },
  getters: {
    isOpenSubsonicEnabled: (state) => state.opensubsonic.enabled,
    isOpenSubsonicConfigured: (state) => state.opensubsonic.configured, // e.g. if URL and username are set
    openSubsonicServerUrl: (state) => state.opensubsonic.server_url,
    openSubsonicUsername: (state) => state.opensubsonic.username,
    // Add other getters as needed, e.g., for connection status

    grantedSpotifyScopes: (state) =>
      state.spotify.webapi_granted_scope?.split(' ') ?? [],
    hasMissingSpotifyScopes: (state) => state.missingSpotifyScopes.length > 0,
    isAuthorizationRequired: (state) =>
      !state.isSpotifyActive || state.hasMissingSpotifyScopes,
    isLastfmActive: (state) => state.lastfm.scrobbling_enabled,
    isLastfmEnabled: (state) => state.lastfm.enabled,
    isSpotifyActive: (state) => state.spotify.webapi_token_valid,
    isSpotifyEnabled: (state) => state.spotify.spotify_installed,
    missingSpotifyScopes(state) {
      const scopes = new Set(state.grantedSpotifyScopes)
      return (
        state.requiredSpotifyScopes.filter((scope) => !scopes.has(scope)) ?? []
      )
    },
    requiredSpotifyScopes: (state) =>
      state.spotify.webapi_required_scope?.split(' ') ?? []
  },
  state: () => ({
    lastfm: {},
    spotify: {},
    opensubsonic: {
      enabled: false, // Is the backend compiled with OpenSubsonic support
      configured: false, // Are settings like URL/user/pass present
      server_url: '',
      username: '',
      // password is not stored in the store, only sent to API
      error: false // If there was an error fetching status
    },
    spotifyTimerId: 0
  })
})
