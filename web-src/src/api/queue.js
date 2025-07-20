import api from '@/api'
import i18n from '@/i18n'
import { useNotificationsStore } from '@/stores/notifications'
import { useQueueStore } from '@/stores/queue'

const { t } = i18n.global
const BASE_URL = './api/queue'

export default {
  async add(params, next = false) {
    if (next) {
      const { current } = useQueueStore()
      if (current?.id) {
        params.position = current.position + 1
      }
    }
    const data = await api.post(`${BASE_URL}/items/add`, null, { params })
    useNotificationsStore().add({
      text: t('server.appended-tracks', { count: data.count }),
      timeout: 2000,
      type: 'info'
    })
    return data
  },
  addExpression(expression, next = false) {
    return this.add({ expression }, next)
  },
  addUri(uris, next = false) {
    // Count the number of URIs by splitting on comma
    const uriCount = uris ? uris.split(',').length : 0
    
    // If more than 50 URIs, use POST body instead of query parameters
    if (uriCount > 50) {
      const params = {}
      if (next) {
        const { current } = useQueueStore()
        if (current?.id) {
          params.position = current.position + 1
        }
      }
      
      // Send uris in POST body as JSON
      const data = api.post(`${BASE_URL}/items/add`, { uris }, { params })
      data.then((result) => {
        useNotificationsStore().add({
          text: t('server.appended-tracks', { count: result.count }),
          timeout: 2000,
          type: 'info'
        })
      })
      return data
    } else {
      // Use original method for 50 or fewer URIs
      return this.add({ uris }, next)
    }
  },
  clear() {
    return api.put(`${BASE_URL}/clear`)
  },
  move(id, position) {
    return api.put(`${BASE_URL}/items/${id}`, null, {
      params: { new_position: position }
    })
  },
  playExpression(expression, shuffle, position) {
    const params = {
      clear: 'true',
      expression,
      playback: 'start',
      playback_from_position: position,
      shuffle
    }
    return api.post(`${BASE_URL}/items/add`, null, { params })
  },
  playUri(uris, shuffle, position) {
    // Count the number of URIs by splitting on comma
    const uriCount = uris ? uris.split(',').length : 0
    
    const baseParams = {
      clear: 'true',
      playback: 'start',
      playback_from_position: position,
      shuffle
    }
    
    // If more than 50 URIs, use POST body instead of query parameters
    if (uriCount > 50) {
      return api.post(`${BASE_URL}/items/add`, { uris }, { params: baseParams })
    } else {
      // Use original method for 50 or fewer URIs
      const params = { ...baseParams, uris }
      return api.post(`${BASE_URL}/items/add`, null, { params })
    }
  },
  remove(id) {
    return api.delete(`${BASE_URL}/items/${id}`)
  },
  async saveToPlaylist(name) {
    const { data } = await api.post(`${BASE_URL}/save`, null, {
      params: { name }
    })
    useNotificationsStore().add({
      text: t('server.queue-saved', { name }),
      timeout: 2000,
      type: 'info'
    })
    return data
  },
  state() {
    return api.get(BASE_URL)
  }
}
