#ifndef __SUBSONIC_H__
#define __SUBSONIC_H__

#include <stdbool.h>
#include <stdint.h>

struct subsonic_status
{
  bool enabled;
  bool logged_in;
  char username[128];
  char server_url[256];
  char api_version[16];
};

struct subsonic_credentials
{
  char username[128];
  char password[128];  // 可以是明文密码或者salt+token
  char salt[32];      // 用于API认证
  char token[64];     // 用于API认证
  bool use_token_auth; // 是否使用token认证
};

struct subsonic_backend
{
  int (*init)(void);
  void (*deinit)(void);
  int (*login)(const char *username, const char *password, const char *server_url, const char **errmsg);
  void (*logout)(void);
  int (*relogin)(void);
  void (*status_get)(struct subsonic_status *status);
  
  // 基本的媒体库操作
  int (*get_music_folders)(void);
  int (*get_artists)(const char *music_folder_id);
  int (*get_artist)(const char *artist_id);
  int (*get_album)(const char *album_id);
  int (*get_song)(const char *song_id);
  
  // 播放列表操作
  int (*get_playlists)(void);
  int (*get_playlist)(const char *playlist_id);
  int (*create_playlist)(const char *name, const char *song_ids);
  int (*update_playlist)(const char *playlist_id, const char *name, const char *song_ids);
  int (*delete_playlist)(const char *playlist_id);
  
  // 搜索
  int (*search)(const char *query, const char *artist_id, const char *album_id);
  
  // 专辑/歌曲评分
  int (*star)(const char *id);
  int (*unstar)(const char *id);
  int (*set_rating)(const char *id, int rating);
  
  // 封面图片
  int (*get_cover_art)(const char *id, int size);
};

// 主要的API函数
int
subsonic_init(void);

void
subsonic_deinit(void);

int
subsonic_login(const char *username, const char *password, const char *server_url, const char **errmsg);

void
subsonic_logout(void);

int
subsonic_relogin(void);

void
subsonic_status_get(struct subsonic_status *status);

// 媒体库操作函数
int
subsonic_get_music_folders(void);

int
subsonic_get_artists(const char *music_folder_id);

int
subsonic_get_artist(const char *artist_id);

int
subsonic_get_album(const char *album_id);

int
subsonic_get_song(const char *song_id);

// 播放列表操作函数
int
subsonic_get_playlists(void);

int
subsonic_get_playlist(const char *playlist_id);

int
subsonic_create_playlist(const char *name, const char *song_ids);

int
subsonic_update_playlist(const char *playlist_id, const char *name, const char *song_ids);

int
subsonic_delete_playlist(const char *playlist_id);

// 搜索函数
int
subsonic_search(const char *query, const char *artist_id, const char *album_id);

// 评分函数
int
subsonic_star(const char *id);

int
subsonic_unstar(const char *id);

int
subsonic_set_rating(const char *id, int rating);

// 封面图片函数
int
subsonic_get_cover_art(const char *id, int size);

// 认证相关函数
int
subsonic_auth_token_generate(const char *password);

int
subsonic_auth_params_create(char **auth_params, const char *params);

#endif /* !__SUBSONIC_H__ */ 