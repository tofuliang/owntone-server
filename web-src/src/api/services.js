import api from '@/api'

export default {
  // OpenSubsonic service
  opensubsonic() {
    return api.get('/api/opensubsonic')
  },

  lastfm() {
    return api.get('./api/lastfm')
  },
  loginLastfm(credentials) {
    return api.post('./api/lastfm-login', credentials)
  },
  logoutLastfm() {
    return api.get('./api/lastfm-logout')
  },
  logoutSpotify() {
    return api.get('./api/spotify-logout')
  },
  spotify() {
    return api.get('./api/spotify')
  }
}
