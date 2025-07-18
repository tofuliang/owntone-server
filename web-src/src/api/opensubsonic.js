import api from '@/api'

export default {
  // Status and connection
  getStatus() {
    return api.get('/api/opensubsonic')
  },

  // Artists
  getArtists() {
    return api.get('/api/opensubsonic/artists')
  },

  getArtist(id) {
    return api.get(`/api/opensubsonic/artists/${id}`)
  },

  // Albums
  getAlbums() {
    return api.get('/api/opensubsonic/albums')
  },

  getAlbum(id) {
    return api.get(`/api/opensubsonic/albums/${id}`)
  },

  // Playlists
  getPlaylists() {
    return api.get('/api/opensubsonic/playlists')
  },

  getPlaylist(id) {
    return api.get(`/api/opensubsonic/playlists/${id}`)
  },

  // Search
  search(query) {
    return api.get('/api/opensubsonic/search', { params: { query } })
  }
}