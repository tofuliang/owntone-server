import { defineStore } from 'pinia'
import { usePlayerStore } from '@/stores/player'
import opensubsonic from '@/api/opensubsonic'
import queue from '@/api/queue'

export const useOpensubsonicStore = defineStore('opensubsonicStore', {
  state: () => ({
    status: {
      enabled: false,
      connected: false,
      server_url: '',
      username: '',
      password: '',
      api_version: '1.16.1',
      client_name: 'OwnTone'
    },
    artists: [],
    albums: [],
    playlists: [],
    currentArtist: null,
    currentAlbum: null,
    currentPlaylist: null,
    searchResults: null,
    loading: false,
    error: null,
    // 新增独立 loading 状态
    artistsLoading: false,
    albumsLoading: false,
    playlistsLoading: false,
    // 新增 currentTab 状态管理
    currentTab: 'artists'
  }),

  getters: {
    isEnabled: (state) => state.status.enabled,
    isConnected: (state) => state.status.connected,
    serverUrl: (state) => state.status.server_url,
    username: (state) => state.status.username
  },

  actions: {
    async initialize() {
      console.log('OpenSubsonic store initialize() called')
      try {
        this.loading = true
        this.error = null
        console.log('Making API call to /api/opensubsonic...')
        const response = await opensubsonic.getStatus()
        console.log('Full API response:', response)
        console.log('Response data:', response.data)
        console.log('Response status:', response.status)
        
        // Handle different response structures
        if (response.data) {
          this.status = response.data
        } else if (response) {
          // Maybe the response is directly the data
          this.status = response
        } else {
          throw new Error('No data in response')
        }
        
        console.log('OpenSubsonic status loaded:', this.status)
        console.log('isConnected after initialize:', this.isConnected)
      } catch (error) {
        console.error('Failed to initialize OpenSubsonic:', error)
        console.error('Error details:', error.response?.data || error.message)
        this.error = 'Failed to initialize OpenSubsonic'
        // Ensure status has default values even on error
        this.status = {
          enabled: false,
          connected: false,
          server_url: '',
          username: '',
          password: '',
          api_version: '1.16.1',
          client_name: 'OwnTone'
        }
        console.log('Set default status after error:', this.status)
      } finally {
        this.loading = false
        console.log('OpenSubsonic initialize() completed. Final status:', this.status)
      }
    },


    async loadArtists() {
      try {
        this.artistsLoading = true
        const response = await opensubsonic.getArtists()
        let items = []
        if (response.data && response.data.items) {
          items = response.data.items
        } else if (response.items) {
          items = response.items
        }
        this.artists = items
      } catch (error) {
        console.error('Failed to load artists:', error)
        this.error = 'Failed to load artists'
      } finally {
        this.artistsLoading = false
      }
    },

    async loadArtist(id) {
      console.log('OpenSubsonic loadArtist() called with ID:', id)
      try {
        this.loading = true
        console.log('Making API call to /api/opensubsonic/artists/' + id)
        const response = await opensubsonic.getArtist(id)
        console.log('Artist API response:', response)
        console.log('Artist response data:', response.data)
        // Handle different response structures - the API returns the artist data directly
        this.currentArtist = response.data || response || null
        console.log('Current artist set to:', this.currentArtist)
      } catch (error) {
        console.error('Failed to load artist:', error)
        console.error('Artist error details:', error.response?.data || error.message)
        this.error = 'Failed to load artist'
      } finally {
        this.loading = false
        console.log('loadArtist() completed')
      }
    },

    async loadAlbums() {
      try {
        this.albumsLoading = true
        const response = await opensubsonic.getAlbums()
        // Handle different response structures
        if (response.data && response.data.items) {
          this.albums = response.data.items
        } else if (response.items) {
          this.albums = response.items
        } else {
          this.albums = []
        }
      } catch (error) {
        console.error('Failed to load albums:', error)
        this.error = 'Failed to load albums'
      } finally {
        this.albumsLoading = false
      }
    },

    async loadAlbum(id) {
      try {
        this.loading = true
        const response = await opensubsonic.getAlbum(id)
        // Handle different response structures - the API returns the album data directly
        this.currentAlbum = response.data || response || null
      } catch (error) {
        console.error('Failed to load album:', error)
        this.error = 'Failed to load album'
      } finally {
        this.loading = false
      }
    },

    async loadPlaylists() {
      try {
        this.playlistsLoading = true
        const response = await opensubsonic.getPlaylists()
        // Handle different response structures
        if (response.data && response.data.items) {
          this.playlists = response.data.items
        } else if (response.items) {
          this.playlists = response.items
        } else {
          this.playlists = []
        }
      } catch (error) {
        console.error('Failed to load playlists:', error)
        this.error = 'Failed to load playlists'
      } finally {
        this.playlistsLoading = false
      }
    },

    async loadPlaylist(id) {
      try {
        this.loading = true
        const response = await opensubsonic.getPlaylist(id)
        // Handle different response structures - the API returns the playlist data directly
        const rawPlaylist = response.data || response || null
        
        // Store the raw playlist data directly, just like album page does
        this.currentPlaylist = rawPlaylist
      } catch (error) {
        console.error('Failed to load playlist:', error)
        this.error = 'Failed to load playlist'
      } finally {
        this.loading = false
      }
    },

    async search(query) {
      try {
        this.loading = true
        this.error = null
        const response = await opensubsonic.search(query)
        this.searchResults = response.data || response || null
      } catch (error) {
        console.error('Failed to search:', error)
        this.error = 'Failed to search'
        this.searchResults = null
      } finally {
        this.loading = false
      }
    },

    clearData() {
      this.artists = []
      this.albums = []
      this.playlists = []
      this.currentArtist = null
      this.currentAlbum = null
      this.currentPlaylist = null
      this.searchResults = null
      this.error = null
    },

    clearError() {
      this.error = null
    },

    setCurrentTab(tab) {
      this.currentTab = tab
    },

    getArtworkUrl(id, size = 300) {
      if (!id || !this.status.server_url) return null
      const salt = Math.random().toString(36).substring(7)
      const token = this.generateToken(this.status.password, salt)
      return `${this.status.server_url}/rest/getCoverArt?id=${id}&size=${size}&u=${this.status.username}&t=${token}&s=${salt}&v=${this.status.api_version}&c=${this.status.client_name}&f=json`
    },

    generateToken(password, salt) {
      // Simple MD5-like hash for demo - in production use proper MD5
      let hash = ''
      const input = password + salt
      for (let i = 0; i < 32; i++) {
        hash += ((input.charCodeAt(i % input.length) * salt.charCodeAt(i % salt.length)) % 16).toString(16)
      }
      return hash
    },

    async addToQueueAndPlay(url, metadata) {
      try {
        // Use queue API to play the track
        await queue.playUri(url, false)
      } catch (error) {
        console.error('Failed to add to queue and play:', error)
        this.error = 'Failed to play track'
      }
    },

    async addToQueue(songs) {
      try {
        // Convert songs to URIs for queue API
        // Handle both string arrays and object arrays
        const uris = songs.map(song => {
          if (typeof song === 'string') {
            return song
          }
          return song.url || song.uri || song.id
        }).filter(uri => uri && uri.trim()).join(',')
        
        if (uris) {
          await queue.addUri(uris)
        }
      } catch (error) {
        console.error('Failed to add to queue:', error)
        this.error = 'Failed to add to queue'
      }
    },

    async replaceQueueAndPlay(songs) {
      try {
        // Convert songs to URIs for queue API
        // Handle both string arrays and object arrays
        const uris = songs.map(song => {
          if (typeof song === 'string') {
            return song
          }
          return song.url || song.uri || song.id
        }).filter(uri => uri && uri.trim()).join(',')
        
        if (uris) {
          await queue.playUri(uris, false)
        }
      } catch (error) {
        console.error('Failed to replace queue and play:', error)
        this.error = 'Failed to play tracks'
      }
    }
  }
})