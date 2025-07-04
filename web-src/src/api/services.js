import api from '@/api'

export default {
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
  },
  openSubsonicStatus() {
    return api.get('./api/opensubsonic/status')
  },
  saveOpenSubsonicConfig(config) {
    return api.put('./api/opensubsonic/config', config)
  },
  testOpenSubsonicConnection(config) {
    // Using POST to send current form data for test, even if it's just a ping
    return api.post('./api/opensubsonic/test', config)
  },
  openSubsonicSearch(params) {
    // params: { query, artistCount, artistOffset, albumCount, albumOffset, songCount, songOffset }
    return api.get('./api/opensubsonic/search', { params })
  },
  openSubsonicGetPlaylists(params) {
    // params: { offset, count }
    return api.get('./api/opensubsonic/playlists', { params })
  },
  openSubsonicGetPlaylist(playlistId) {
    return api.get(`./api/opensubsonic/playlist/${playlistId}`)
  },
  openSubsonicGetAlbum(albumId) {
    return api.get(`./api/opensubsonic/album/${albumId}`)
  },
  openSubsonicGetArtist(artistId) {
    return api.get(`./api/opensubsonic/artist/${artistId}`)
  },
  openSubsonicGetSong(songId) {
    return api.get(`./api/opensubsonic/song/${songId}`)
  },
  openSubsonicGetCoverArtConcreteUrl(mediaId, size = 0) {
    let url = `./api/opensubsonic/coverarturl/${mediaId}`
    if (size > 0) {
      url += `?size=${size}`
    }
    return api.get(url) // This will return a promise resolving to { url: "..." }
  }
}
