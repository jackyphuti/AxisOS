/*
 * AxisOS Native wlroots Wayland Compositor
 *
 * Implements a modern, lightweight, GPU-accelerated Wayland desktop compositor
 * powered by wlroots and wlr_scene.
 *
 * Initialization Sequence:
 *   1. Initialize the Wayland Display (wl_display_create)
 *   2. Initialize the Backend (wlr_backend_autocreate)
 *   3. Set Up Renderer and Allocator (wlr_renderer_autocreate, wlr_allocator_autocreate, wlr_compositor_create)
 *   4. Initialize Shell and Scene Graph (wlr_xdg_shell_create, wlr_layer_shell_v1_create, wlr_scene_create)
 *   5. Wire Up Hardware Listeners (new_input, new_output, cursor, keyboard)
 *   6. Start the Server (wlr_backend_start, wl_display_run)
 */

#define _POSIX_C_SOURCE 200809L
#define WLR_USE_UNSTABLE 1

#include <assert.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#include <wayland-server-core.h>
#include <wlr/backend.h>
#include <wlr/backend/libinput.h>
#include <libinput.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/types/wlr_layer_shell_v1.h>
#include <wlr/types/wlr_foreign_toplevel_management_v1.h>
#include <wlr/util/log.h>
#include <wlr/version.h>
#include <xkbcommon/xkbcommon.h>

#if defined(WLR_VERSION_NUM) && WLR_VERSION_NUM >= ((0 << 16) | (16 << 8) | 0)
#define AXIS_WLR_016_PLUS 1
#define AXIS_SCENE_PARENT(server) (&(server)->scene->tree)
#define axis_wlr_xdg_toplevel_set_activated(surf, toplevel, act) wlr_xdg_toplevel_set_activated((toplevel), (act))
#define axis_wlr_xdg_toplevel_set_size(surf, toplevel, w, h) wlr_xdg_toplevel_set_size((toplevel), (w), (h))
#define axis_wlr_xdg_toplevel_set_fullscreen(surf, toplevel, fs) wlr_xdg_toplevel_set_fullscreen((toplevel), (fs))
#define axis_wlr_xdg_toplevel_set_maximized(surf, toplevel, max) wlr_xdg_toplevel_set_maximized((toplevel), (max))
#define axis_wlr_xdg_toplevel_send_close(surf, toplevel) wlr_xdg_toplevel_send_close((toplevel))
#else
#define AXIS_WLR_016_PLUS 0
#define AXIS_SCENE_PARENT(server) (&(server)->scene->node)
#define axis_wlr_xdg_toplevel_set_activated(surf, toplevel, act) wlr_xdg_toplevel_set_activated((surf), (act))
#define axis_wlr_xdg_toplevel_set_size(surf, toplevel, w, h) wlr_xdg_toplevel_set_size((surf), (w), (h))
#define axis_wlr_xdg_toplevel_set_fullscreen(surf, toplevel, fs) wlr_xdg_toplevel_set_fullscreen((surf), (fs))
#define axis_wlr_xdg_toplevel_set_maximized(surf, toplevel, max) wlr_xdg_toplevel_set_maximized((surf), (max))
#define axis_wlr_xdg_toplevel_send_close(surf, toplevel) wlr_xdg_toplevel_send_close((surf))
#endif

#if defined(WLR_VERSION_NUM) && WLR_VERSION_NUM >= ((0 << 16) | (17 << 8) | 0)
#define AXIS_WLR_017_PLUS 1
#define wlr_xdg_surface_from_wlr_surface wlr_xdg_surface_try_from_wlr_surface
#define axis_wlr_scene_output_commit(so) wlr_scene_output_commit((so), NULL)
#else
#define AXIS_WLR_017_PLUS 0
#define axis_wlr_scene_output_commit(so) wlr_scene_output_commit((so))
#endif

enum axis_cursor_mode {
	AXIS_CURSOR_PASSTHROUGH,
	AXIS_CURSOR_MOVE,
	AXIS_CURSOR_RESIZE,
};

struct axis_server {
	struct wl_display *wl_display;
	struct wlr_backend *backend;
	struct wlr_renderer *renderer;
	struct wlr_allocator *allocator;
	struct wlr_scene *scene;

	struct wlr_xdg_shell *xdg_shell;
	struct wl_listener new_xdg_surface;
	struct wl_list views;

	struct wlr_layer_shell_v1 *layer_shell;
	struct wl_listener new_layer_surface;
	struct wl_list layer_surfaces;

	struct wlr_foreign_toplevel_manager_v1 *foreign_toplevel_mgr;

	struct wlr_cursor *cursor;
	struct wlr_xcursor_manager *cursor_mgr;
	struct wl_listener cursor_motion;
	struct wl_listener cursor_motion_absolute;
	struct wl_listener cursor_button;
	struct wl_listener cursor_axis;
	struct wl_listener cursor_frame;

	struct wlr_seat *seat;
	struct wl_listener new_input;
	struct wl_listener request_cursor;
	struct wl_listener request_set_selection;
	struct wl_list keyboards;

	enum axis_cursor_mode cursor_mode;
	struct axis_view *grabbed_view;
	double grab_x, grab_y;
	struct wlr_box grab_geobox;
	uint32_t resize_edges;

	struct wlr_output_layout *output_layout;
	struct wl_list outputs;
	struct wl_listener new_output;
};

struct axis_output {
	struct wl_list link;
	struct axis_server *server;
	struct wlr_output *wlr_output;
	struct wl_listener frame;
	struct wl_listener destroy;
};

struct axis_view {
	struct wl_list link;
	struct axis_server *server;
	struct wlr_xdg_surface *xdg_surface;
	struct wlr_xdg_toplevel *xdg_toplevel;
	struct wlr_scene_node *scene_node;
	struct wlr_scene_rect *border[4];
	struct wl_listener map;
	struct wl_listener unmap;
	struct wl_listener destroy;
	struct wl_listener surface_commit;
	struct wl_listener request_move;
	struct wl_listener request_resize;
	struct wl_listener request_maximize;
	struct wl_listener request_fullscreen;
	struct wl_listener set_title;
	struct wl_listener set_app_id;

	struct wlr_foreign_toplevel_handle_v1 *toplevel_handle;
	struct wl_listener toplevel_request_maximize;
	struct wl_listener toplevel_request_minimize;
	struct wl_listener toplevel_request_activate;
	struct wl_listener toplevel_request_fullscreen;
	struct wl_listener toplevel_request_close;

	int x, y;
	bool is_maximized;
	bool is_fullscreen;
	struct wlr_box saved_geometry;
};

struct axis_layer_surface {
	struct wl_list link;
	struct axis_server *server;
	struct wlr_layer_surface_v1 *layer_surface;
	struct wlr_scene_node *scene_node;
	struct wl_listener map;
	struct wl_listener unmap;
	struct wl_listener destroy;
	struct wl_listener surface_commit;
	int x, y;
};

struct axis_keyboard {
	struct wl_list link;
	struct axis_server *server;
	struct wlr_input_device *device;
	struct wl_listener modifiers;
	struct wl_listener key;
	struct wl_listener destroy;
};

/* Forward declarations */
static void focus_view(struct axis_view *view, struct wlr_surface *surface);
static void focus_next_view(struct axis_server *server);
static void spawn_command(const char *cmd);
static void xdg_toplevel_request_maximize(struct wl_listener *listener, void *data);
static void xdg_toplevel_request_fullscreen(struct wl_listener *listener, void *data);
static void axis_view_update_borders(struct axis_view *view);

/* Window border colors */
static const float border_color_inactive[4] = { 0.18f, 0.20f, 0.29f, 0.65f };
static const float border_color_active[4]   = { 0.49f, 0.58f, 1.0f,  0.90f };

static void axis_view_update_borders(struct axis_view *view) {
	if (!view || !view->xdg_surface) {
		return;
	}
	for (int i = 0; i < 4; i++) {
		if (!view->border[i]) return;
	}

	if (view->is_fullscreen) {
		for (int i = 0; i < 4; i++) {
			wlr_scene_rect_set_size(view->border[i], 0, 0);
		}
		return;
	}

	struct wlr_box geom;
	wlr_xdg_surface_get_geometry(view->xdg_surface, &geom);

	int x = view->x + geom.x;
	int y = view->y + geom.y;
	int width = geom.width;
	int height = geom.height;

	/* Top: x, y - 2, width, 2 */
	wlr_scene_node_set_position(&view->border[0]->node, x, y - 2);
	wlr_scene_rect_set_size(view->border[0], width, 2);

	/* Bottom: x, y + height, width, 2 */
	wlr_scene_node_set_position(&view->border[1]->node, x, y + height);
	wlr_scene_rect_set_size(view->border[1], width, 2);

	/* Left: x - 2, y - 2, 2, height + 4 */
	wlr_scene_node_set_position(&view->border[2]->node, x - 2, y - 2);
	wlr_scene_rect_set_size(view->border[2], 2, height + 4);

	/* Right: x + width, y - 2, 2, height + 4 */
	wlr_scene_node_set_position(&view->border[3]->node, x + width, y - 2);
	wlr_scene_rect_set_size(view->border[3], 2, height + 4);
}

static void focus_view(struct axis_view *view, struct wlr_surface *surface) {
	if (view == NULL) {
		return;
	}
	struct axis_server *server = view->server;
	struct wlr_seat *seat = server->seat;
	struct wlr_surface *prev_surface = seat->keyboard_state.focused_surface;
	if (prev_surface == surface) {
		return;
	}
	if (prev_surface) {
		struct wlr_xdg_surface *previous = wlr_xdg_surface_from_wlr_surface(prev_surface);
		if (previous && previous->role == WLR_XDG_SURFACE_ROLE_TOPLEVEL && previous->toplevel) {
			axis_wlr_xdg_toplevel_set_activated(previous, previous->toplevel, false);
		}
	}

	/* Reset unfocused windows to subtle slate */
	struct axis_view *v;
	wl_list_for_each(v, &server->views, link) {
		if (v != view) {
			for (int i = 0; i < 4; i++) {
				if (v->border[i]) {
					wlr_scene_rect_set_color(v->border[i], border_color_inactive);
				}
			}
		}
	}

	/* Move view to top of scene */
	wlr_scene_node_raise_to_top(view->scene_node);
	/* Set active window border color to vibrant AxisOS accent blue and raise borders */
	for (int i = 0; i < 4; i++) {
		if (view->border[i]) {
			wlr_scene_node_raise_to_top(&view->border[i]->node);
			wlr_scene_rect_set_color(view->border[i], border_color_active);
		}
	}
	wl_list_remove(&view->link);
	wl_list_insert(&server->views, &view->link);

	if (view->xdg_surface->role == WLR_XDG_SURFACE_ROLE_TOPLEVEL && view->xdg_surface->toplevel) {
		axis_wlr_xdg_toplevel_set_activated(view->xdg_surface, view->xdg_surface->toplevel, true);
	}

	/* Update foreign toplevel manager activation states */
	if (view->toplevel_handle) {
		wlr_foreign_toplevel_handle_v1_set_activated(view->toplevel_handle, true);
	}
	wl_list_for_each(v, &server->views, link) {
		if (v != view && v->toplevel_handle) {
			wlr_foreign_toplevel_handle_v1_set_activated(v->toplevel_handle, false);
		}
	}

	struct wlr_keyboard *keyboard = wlr_seat_get_keyboard(seat);
	if (keyboard != NULL) {
		wlr_seat_keyboard_notify_enter(seat, view->xdg_surface->surface,
			keyboard->keycodes, keyboard->num_keycodes, &keyboard->modifiers);
	}
}

static void focus_next_view(struct axis_server *server) {
	if (wl_list_empty(&server->views)) {
		return;
	}
	/* Cycle to the last view and bring to front */
	struct axis_view *next_view = wl_container_of(server->views.prev, next_view, link);
	if (next_view && next_view->xdg_surface && next_view->xdg_surface->surface) {
		focus_view(next_view, next_view->xdg_surface->surface);
	}
}

static void spawn_command(const char *cmd) {
	if (!cmd || !*cmd) return;
	pid_t pid = fork();
	if (pid == 0) {
		/* Child process */
		setsid();
		setenv("GDK_BACKEND", "wayland", 1);
		setenv("MOZ_ENABLE_WAYLAND", "1", 1);
		setenv("OZONE_PLATFORM", "wayland", 1);
		setenv("WLR_RENDERER_ALLOW_SOFTWARE", "1", 1);
		setenv("WLR_NO_HARDWARE_CURSORS", "1", 1);
		const char *libgl = getenv("LIBGL_ALWAYS_SOFTWARE");
		if (libgl) {
			setenv("LIBGL_ALWAYS_SOFTWARE", libgl, 1);
		}
		execl("/bin/sh", "/bin/sh", "-c", cmd, (void *)NULL);
		_exit(1);
	}
}

static void axis_set_taskbars_visible(struct axis_server *server, bool visible) {
	struct axis_layer_surface *surface;
	wl_list_for_each(surface, &server->layer_surfaces, link) {
		if (surface->layer_surface && surface->layer_surface->current.layer != ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND) {
			wlr_scene_node_set_enabled(surface->scene_node, visible);
		}
	}
}

static bool axis_has_maximized_view(struct axis_server *server, struct axis_view *ignore_view) {
	struct axis_view *v;
	wl_list_for_each(v, &server->views, link) {
		if (v != ignore_view && v->is_maximized) {
			return true;
		}
	}
	return false;
}

/* ========================================================================= */
/* Keyboard Handling & Keybindings                                          */
/* ========================================================================= */
static bool handle_keybinding(struct axis_server *server, uint32_t modifiers, xkb_keysym_t sym) {
	bool is_super = (modifiers & WLR_MODIFIER_LOGO) != 0;
	bool is_alt = (modifiers & WLR_MODIFIER_ALT) != 0;
	bool is_ctrl = (modifiers & WLR_MODIFIER_CTRL) != 0;
	bool is_shift = (modifiers & WLR_MODIFIER_SHIFT) != 0;

	/* Super + Return or Super + T: Terminal */
	if (is_super && (sym == XKB_KEY_Return || sym == XKB_KEY_KP_Enter || sym == XKB_KEY_t || sym == XKB_KEY_T)) {
		spawn_command("axis-terminal &");
		return true;
	}

	/* Super + D or Super + Space: Axis App Launcher */
	if (is_super && (sym == XKB_KEY_d || sym == XKB_KEY_D || sym == XKB_KEY_space)) {
		spawn_command("pkill -USR1 -x axis-shell || pkill -USR1 -x axisos-shell || wofi --show drun --prompt 'Search Apps...' &");
		return true;
	}

	/* Super + H: Axis Welcome & Hardware Driver / Codec Manager */
	if (is_super && (sym == XKB_KEY_h || sym == XKB_KEY_H)) {
		spawn_command("axis-welcome &");
		return true;
	}

	/* Super + G: Steam Gaming Client */
	if (is_super && (sym == XKB_KEY_g || sym == XKB_KEY_G)) {
		spawn_command("steam &");
		return true;
	}

	/* Super + B or Super + W: Web Browser */
	if (is_super && (sym == XKB_KEY_b || sym == XKB_KEY_B || sym == XKB_KEY_w || sym == XKB_KEY_W)) {
		spawn_command("chromium --ozone-platform=wayland &");
		return true;
	}

	/* Super + E: File Manager */
	if (is_super && (sym == XKB_KEY_e || sym == XKB_KEY_E)) {
		spawn_command("nautilus &");
		return true;
	}

	/* Super + L: Lock Session */
	if (is_super && (sym == XKB_KEY_l || sym == XKB_KEY_L)) {
		spawn_command("loginctl lock-session &");
		return true;
	}

	/* Alt + Tab: Window Cycling */
	if (is_alt && sym == XKB_KEY_Tab) {
		focus_next_view(server);
		return true;
	}

	/* Super + Q or Alt + F4: Close active window */
	if ((is_super && (sym == XKB_KEY_q || sym == XKB_KEY_Q)) ||
	    (is_alt && sym == XKB_KEY_F4)) {
		if (!wl_list_empty(&server->views)) {
			struct axis_view *current = wl_container_of(server->views.next, current, link);
			if (current && current->xdg_surface && current->xdg_surface->toplevel) {
				axis_wlr_xdg_toplevel_send_close(current->xdg_surface, current->xdg_surface->toplevel);
			}
		}
		return true;
	}

	/* Super + F: Fullscreen Toggle */
	if (is_super && (sym == XKB_KEY_f || sym == XKB_KEY_F)) {
		if (!wl_list_empty(&server->views)) {
			struct axis_view *current = wl_container_of(server->views.next, current, link);
			if (current && current->xdg_surface && current->xdg_surface->toplevel) {
				struct axis_view *view = current;
				struct wlr_output *output = wlr_output_layout_get_center_output(view->server->output_layout);
				if (output) {
					if (!view->is_fullscreen) {
						view->saved_geometry.x = view->x;
						view->saved_geometry.y = view->y;
						struct wlr_box geo_box;
						wlr_xdg_surface_get_geometry(view->xdg_surface, &geo_box);
						view->saved_geometry.width = geo_box.width;
						view->saved_geometry.height = geo_box.height;

						wlr_scene_node_set_position(view->scene_node, 0, 0);
						axis_wlr_xdg_toplevel_set_size(view->xdg_surface, view->xdg_surface->toplevel, output->width, output->height);
						axis_wlr_xdg_toplevel_set_fullscreen(view->xdg_surface, view->xdg_surface->toplevel, true);
						view->is_fullscreen = true;
						axis_view_update_borders(view);
					} else {
						axis_wlr_xdg_toplevel_set_size(view->xdg_surface, view->xdg_surface->toplevel, view->saved_geometry.width, view->saved_geometry.height);
						wlr_scene_node_set_position(view->scene_node, view->saved_geometry.x, view->saved_geometry.y);
						axis_wlr_xdg_toplevel_set_fullscreen(view->xdg_surface, view->xdg_surface->toplevel, false);
						view->is_fullscreen = false;
						axis_view_update_borders(view);
					}
				}
			}
		}
		return true;
	}

	/* Super + M: Maximize Toggle */
	if (is_super && (sym == XKB_KEY_m || sym == XKB_KEY_M)) {
		if (!wl_list_empty(&server->views)) {
			struct axis_view *current = wl_container_of(server->views.next, current, link);
			if (current && current->xdg_surface && current->xdg_surface->toplevel) {
				struct axis_view *view = current;
				if (view->is_maximized) {
					axis_wlr_xdg_toplevel_set_size(view->xdg_surface, view->xdg_surface->toplevel, view->saved_geometry.width, view->saved_geometry.height);
					wlr_scene_node_set_position(view->scene_node, view->saved_geometry.x, view->saved_geometry.y);
					axis_wlr_xdg_toplevel_set_maximized(view->xdg_surface, view->xdg_surface->toplevel, false);
					view->is_maximized = false;
					if (!axis_has_maximized_view(view->server, NULL)) {
						axis_set_taskbars_visible(view->server, true);
					}
					axis_view_update_borders(view);
				} else {
					struct wlr_output *output = wlr_output_layout_get_center_output(view->server->output_layout);
					if (output) {
						view->saved_geometry.x = view->x;
						view->saved_geometry.y = view->y;
						struct wlr_box geo_box;
						wlr_xdg_surface_get_geometry(view->xdg_surface, &geo_box);
						view->saved_geometry.width = geo_box.width;
						view->saved_geometry.height = geo_box.height;

						/* Auto-hide taskbar on maximize */
						axis_set_taskbars_visible(view->server, false);
						wlr_scene_node_set_position(view->scene_node, 0, 0);
						axis_wlr_xdg_toplevel_set_size(view->xdg_surface, view->xdg_surface->toplevel, output->width, output->height);
						axis_wlr_xdg_toplevel_set_maximized(view->xdg_surface, view->xdg_surface->toplevel, true);
						view->is_maximized = true;
						axis_view_update_borders(view);
					}
				}
			}
		}
		return true;
	}

	/* Super + Shift + E or Ctrl + Alt + Delete: Terminate Compositor */
	if ((is_super && is_shift && (sym == XKB_KEY_e || sym == XKB_KEY_E)) ||
	    (is_ctrl && is_alt && sym == XKB_KEY_Delete)) {
		wl_display_terminate(server->wl_display);
		return true;
	}

	/* Multimedia Audio Keys */
	if (sym == XKB_KEY_XF86AudioRaiseVolume) {
		spawn_command("amixer -q sset Master 5%+ &");
		return true;
	}
	if (sym == XKB_KEY_XF86AudioLowerVolume) {
		spawn_command("amixer -q sset Master 5%- &");
		return true;
	}
	if (sym == XKB_KEY_XF86AudioMute) {
		spawn_command("amixer -q sset Master toggle &");
		return true;
	}

	/* Brightness Keys */
	if (sym == XKB_KEY_XF86MonBrightnessUp) {
		spawn_command("brightnessctl -q set +10% &");
		return true;
	}
	if (sym == XKB_KEY_XF86MonBrightnessDown) {
		spawn_command("brightnessctl -q set 10%- &");
		return true;
	}

	return false;
}

static void keyboard_handle_modifiers(struct wl_listener *listener, void *data) {
	(void)data;
	struct axis_keyboard *keyboard = wl_container_of(listener, keyboard, modifiers);
#if AXIS_WLR_017_PLUS
	struct wlr_keyboard *wlr_kb = wlr_keyboard_from_input_device(keyboard->device);
	wlr_seat_set_keyboard(keyboard->server->seat, wlr_kb);
	wlr_seat_keyboard_notify_modifiers(keyboard->server->seat,
		&wlr_kb->modifiers);
#elif AXIS_WLR_016_PLUS
	wlr_seat_set_keyboard(keyboard->server->seat, keyboard->device->keyboard);
	wlr_seat_keyboard_notify_modifiers(keyboard->server->seat,
		&keyboard->device->keyboard->modifiers);
#else
	wlr_seat_set_keyboard(keyboard->server->seat, keyboard->device);
	wlr_seat_keyboard_notify_modifiers(keyboard->server->seat,
		&keyboard->device->keyboard->modifiers);
#endif
}

static void keyboard_handle_key(struct wl_listener *listener, void *data) {
	struct axis_keyboard *keyboard = wl_container_of(listener, keyboard, key);
	struct axis_server *server = keyboard->server;
#if AXIS_WLR_016_PLUS
	struct wlr_keyboard_key_event *event = data;
#else
	struct wlr_event_keyboard_key *event = data;
#endif
	struct wlr_seat *seat = server->seat;

#if AXIS_WLR_017_PLUS
	struct wlr_keyboard *wlr_kb = wlr_keyboard_from_input_device(keyboard->device);
#else
	struct wlr_keyboard *wlr_kb = keyboard->device->keyboard;
#endif

	uint32_t keycode = event->keycode + 8;
	const xkb_keysym_t *syms;
	int nsyms = xkb_state_key_get_syms(wlr_kb->xkb_state, keycode, &syms);

	bool handled = false;
	uint32_t modifiers = wlr_keyboard_get_modifiers(wlr_kb);
	if (event->state == WL_KEYBOARD_KEY_STATE_PRESSED) {
		for (int i = 0; i < nsyms; i++) {
			handled = handle_keybinding(server, modifiers, syms[i]);
			if (handled) break;
		}
	}

	if (!handled) {
#if AXIS_WLR_017_PLUS
		wlr_seat_set_keyboard(seat, wlr_kb);
#elif AXIS_WLR_016_PLUS
		wlr_seat_set_keyboard(seat, keyboard->device->keyboard);
#else
		wlr_seat_set_keyboard(seat, keyboard->device);
#endif
		wlr_seat_keyboard_notify_key(seat, event->time_msec,
			event->keycode, event->state);
	}
}

/* ========================================================================= */
/* Cursor & Pointer Handling                                                 */
/* ========================================================================= */
static struct axis_view *desktop_view_at(struct axis_server *server,
		double lx, double ly, struct wlr_surface **surface, double *sx, double *sy) {
#if AXIS_WLR_017_PLUS
	struct wlr_scene_node *node = wlr_scene_node_at(&server->scene->tree.node, lx, ly, sx, sy);
	if (node == NULL || node->type != WLR_SCENE_NODE_BUFFER) {
		return NULL;
	}
	struct wlr_scene_buffer *scene_buffer = wlr_scene_buffer_from_node(node);
	struct wlr_scene_surface *scene_surface = wlr_scene_surface_try_from_buffer(scene_buffer);
	if (!scene_surface) {
		return NULL;
	}
	*surface = scene_surface->surface;
	while (node != NULL && node->data == NULL) {
		node = node->parent ? &node->parent->node : NULL;
	}
	return node ? node->data : NULL;
#else
	struct wlr_scene_node *node = wlr_scene_node_at(&server->scene->node, lx, ly, sx, sy);
	if (node == NULL || node->type != WLR_SCENE_NODE_SURFACE) {
		return NULL;
	}
	*surface = wlr_scene_surface_from_node(node)->surface;
	while (node != NULL && node->data == NULL) {
		node = node->parent;
	}
	return node ? node->data : NULL;
#endif
}

static void process_cursor_move(struct axis_server *server) {
	struct axis_view *view = server->grabbed_view;
	view->x = server->cursor->x - server->grab_x;
	view->y = server->cursor->y - server->grab_y;
	wlr_scene_node_set_position(view->scene_node, view->x, view->y);
	axis_view_update_borders(view);
}

static void process_cursor_resize(struct axis_server *server) {
	struct axis_view *view = server->grabbed_view;
	double border_x = server->cursor->x - server->grab_x;
	double border_y = server->cursor->y - server->grab_y;
	int new_left = server->grab_geobox.x;
	int new_right = server->grab_geobox.x + server->grab_geobox.width;
	int new_top = server->grab_geobox.y;
	int new_bottom = server->grab_geobox.y + server->grab_geobox.height;

	if (server->resize_edges & WLR_EDGE_TOP) {
		new_top = border_y;
		if (new_top >= new_bottom) new_top = new_bottom - 1;
	} else if (server->resize_edges & WLR_EDGE_BOTTOM) {
		new_bottom = border_y;
		if (new_bottom <= new_top) new_bottom = new_top + 1;
	}
	if (server->resize_edges & WLR_EDGE_LEFT) {
		new_left = border_x;
		if (new_left >= new_right) new_left = new_right - 1;
	} else if (server->resize_edges & WLR_EDGE_RIGHT) {
		new_right = border_x;
		if (new_right <= new_left) new_right = new_left + 1;
	}

	struct wlr_box geo_box;
	wlr_xdg_surface_get_geometry(view->xdg_surface, &geo_box);
	view->x = new_left - geo_box.x;
	view->y = new_top - geo_box.y;
	wlr_scene_node_set_position(view->scene_node, view->x, view->y);

	int new_width = new_right - new_left;
	int new_height = new_bottom - new_top;
	axis_wlr_xdg_toplevel_set_size(view->xdg_surface, view->xdg_surface->toplevel, new_width, new_height);
	axis_view_update_borders(view);
}

static void process_cursor_motion(struct axis_server *server, uint32_t time) {
	if (server->cursor_mode == AXIS_CURSOR_MOVE) {
		process_cursor_move(server);
		return;
	} else if (server->cursor_mode == AXIS_CURSOR_RESIZE) {
		process_cursor_resize(server);
		return;
	}

	double sx, sy;
	struct wlr_surface *surface = NULL;
	struct axis_view *view = desktop_view_at(server, server->cursor->x, server->cursor->y, &surface, &sx, &sy);
	if (!view) {
#if AXIS_WLR_017_PLUS
		wlr_cursor_set_xcursor(server->cursor, server->cursor_mgr, "left_ptr");
#else
		wlr_xcursor_manager_set_cursor_image(server->cursor_mgr, "left_ptr", server->cursor);
#endif
	}
	if (surface) {
		wlr_seat_pointer_notify_enter(server->seat, surface, sx, sy);
		wlr_seat_pointer_notify_motion(server->seat, time, sx, sy);
	} else {
		wlr_seat_pointer_clear_focus(server->seat);
	}
}

static void server_cursor_motion(struct wl_listener *listener, void *data) {
	struct axis_server *server = wl_container_of(listener, server, cursor_motion);
#if AXIS_WLR_016_PLUS
	struct wlr_pointer_motion_event *event = data;
	wlr_cursor_move(server->cursor, &event->pointer->base, event->delta_x, event->delta_y);
#else
	struct wlr_event_pointer_motion *event = data;
	wlr_cursor_move(server->cursor, event->device, event->delta_x, event->delta_y);
#endif
	process_cursor_motion(server, event->time_msec);
}

static void server_cursor_motion_absolute(struct wl_listener *listener, void *data) {
	struct axis_server *server = wl_container_of(listener, server, cursor_motion_absolute);
#if AXIS_WLR_016_PLUS
	struct wlr_pointer_motion_absolute_event *event = data;
	wlr_cursor_warp_absolute(server->cursor, &event->pointer->base, event->x, event->y);
#else
	struct wlr_event_pointer_motion_absolute *event = data;
	wlr_cursor_warp_absolute(server->cursor, event->device, event->x, event->y);
#endif
	process_cursor_motion(server, event->time_msec);
}

static void server_cursor_button(struct wl_listener *listener, void *data) {
	struct axis_server *server = wl_container_of(listener, server, cursor_button);
#if AXIS_WLR_016_PLUS
	struct wlr_pointer_button_event *event = data;
#else
	struct wlr_event_pointer_button *event = data;
#endif
	wlr_seat_pointer_notify_button(server->seat, event->time_msec, event->button, event->state);
	double sx, sy;
	struct wlr_surface *surface = NULL;
	struct axis_view *view = desktop_view_at(server, server->cursor->x, server->cursor->y, &surface, &sx, &sy);

	if (event->state == WLR_BUTTON_RELEASED) {
		server->cursor_mode = AXIS_CURSOR_PASSTHROUGH;
	} else if (view) {
		focus_view(view, surface);
	}
}

static void server_cursor_axis(struct wl_listener *listener, void *data) {
	struct axis_server *server = wl_container_of(listener, server, cursor_axis);
#if AXIS_WLR_016_PLUS
	struct wlr_pointer_axis_event *event = data;
#else
	struct wlr_event_pointer_axis *event = data;
#endif
	wlr_seat_pointer_notify_axis(server->seat, event->time_msec, event->orientation,
		event->delta, event->delta_discrete, event->source);
}

static void server_cursor_frame(struct wl_listener *listener, void *data) {
	(void)data;
	struct axis_server *server = wl_container_of(listener, server, cursor_frame);
	wlr_seat_pointer_notify_frame(server->seat);
}

/* ========================================================================= */
/* Foreign Toplevel Callbacks & Handlers                                     */
/* ========================================================================= */
static void toplevel_handle_request_activate(struct wl_listener *listener, void *data) {
	(void)data;
	struct axis_view *view = wl_container_of(listener, view, toplevel_request_activate);
	if (view && view->xdg_surface && view->xdg_surface->surface) {
		focus_view(view, view->xdg_surface->surface);
	}
}

static void toplevel_handle_request_close(struct wl_listener *listener, void *data) {
	(void)data;
	struct axis_view *view = wl_container_of(listener, view, toplevel_request_close);
	if (view && view->xdg_surface && view->xdg_surface->toplevel) {
		wlr_xdg_toplevel_send_close(view->xdg_surface);
	}
}

static void toplevel_handle_request_maximize(struct wl_listener *listener, void *data) {
	struct axis_view *view = wl_container_of(listener, view, toplevel_request_maximize);
	struct wlr_foreign_toplevel_handle_v1_maximized_event *event = data;
	if (event && event->maximized != view->is_maximized) {
		xdg_toplevel_request_maximize(&view->request_maximize, NULL);
	}
}

static void toplevel_handle_request_fullscreen(struct wl_listener *listener, void *data) {
	struct axis_view *view = wl_container_of(listener, view, toplevel_request_fullscreen);
	struct wlr_foreign_toplevel_handle_v1_fullscreen_event *event = data;
	if (event && event->fullscreen != view->is_fullscreen) {
		xdg_toplevel_request_fullscreen(&view->request_fullscreen, NULL);
	}
}

static void xdg_toplevel_set_title(struct wl_listener *listener, void *data) {
	(void)data;
	struct axis_view *view = wl_container_of(listener, view, set_title);
	if (view->toplevel_handle && view->xdg_surface->toplevel->title) {
		wlr_foreign_toplevel_handle_v1_set_title(view->toplevel_handle, view->xdg_surface->toplevel->title);
	}
}

static void xdg_toplevel_set_app_id(struct wl_listener *listener, void *data) {
	(void)data;
	struct axis_view *view = wl_container_of(listener, view, set_app_id);
	if (view->toplevel_handle && view->xdg_surface->toplevel->app_id) {
		wlr_foreign_toplevel_handle_v1_set_app_id(view->toplevel_handle, view->xdg_surface->toplevel->app_id);
	}
}

/* ========================================================================= */
/* XDG Surfaces (Application Windows)                                        */
/* ========================================================================= */
static void xdg_toplevel_map(struct wl_listener *listener, void *data) {
	(void)data;
	struct axis_view *view = wl_container_of(listener, view, map);
	wl_list_insert(&view->server->views, &view->link);

	if (view->server->foreign_toplevel_mgr) {
		view->toplevel_handle = wlr_foreign_toplevel_handle_v1_create(view->server->foreign_toplevel_mgr);
		if (view->toplevel_handle) {
			struct wlr_xdg_toplevel *toplevel = view->xdg_surface->toplevel;
			if (toplevel->title) {
				wlr_foreign_toplevel_handle_v1_set_title(view->toplevel_handle, toplevel->title);
			}
			if (toplevel->app_id) {
				wlr_foreign_toplevel_handle_v1_set_app_id(view->toplevel_handle, toplevel->app_id);
			}
			wlr_foreign_toplevel_handle_v1_set_maximized(view->toplevel_handle, view->is_maximized);
			wlr_foreign_toplevel_handle_v1_set_fullscreen(view->toplevel_handle, view->is_fullscreen);

			view->toplevel_request_activate.notify = toplevel_handle_request_activate;
			wl_signal_add(&view->toplevel_handle->events.request_activate, &view->toplevel_request_activate);
			view->toplevel_request_close.notify = toplevel_handle_request_close;
			wl_signal_add(&view->toplevel_handle->events.request_close, &view->toplevel_request_close);
			view->toplevel_request_maximize.notify = toplevel_handle_request_maximize;
			wl_signal_add(&view->toplevel_handle->events.request_maximize, &view->toplevel_request_maximize);
			view->toplevel_request_fullscreen.notify = toplevel_handle_request_fullscreen;
			wl_signal_add(&view->toplevel_handle->events.request_fullscreen, &view->toplevel_request_fullscreen);
		}
	}

	focus_view(view, view->xdg_surface->surface);
	axis_view_update_borders(view);
}

static void xdg_toplevel_unmap(struct wl_listener *listener, void *data) {
	(void)data;
	struct axis_view *view = wl_container_of(listener, view, unmap);
	if (view->toplevel_handle) {
		wl_list_remove(&view->toplevel_request_activate.link);
		wl_list_remove(&view->toplevel_request_close.link);
		wl_list_remove(&view->toplevel_request_maximize.link);
		wl_list_remove(&view->toplevel_request_fullscreen.link);
		wlr_foreign_toplevel_handle_v1_destroy(view->toplevel_handle);
		view->toplevel_handle = NULL;
	}
	if (view->is_maximized) {
		view->is_maximized = false;
		if (!axis_has_maximized_view(view->server, view)) {
			axis_set_taskbars_visible(view->server, true);
		}
	}
	for (int i = 0; i < 4; i++) {
		if (view->border[i]) {
			wlr_scene_rect_set_size(view->border[i], 0, 0);
		}
	}
	wl_list_remove(&view->link);
}

static void xdg_toplevel_destroy(struct wl_listener *listener, void *data) {
	(void)data;
	struct axis_view *view = wl_container_of(listener, view, destroy);
	if (view->toplevel_handle) {
		wl_list_remove(&view->toplevel_request_activate.link);
		wl_list_remove(&view->toplevel_request_close.link);
		wl_list_remove(&view->toplevel_request_maximize.link);
		wl_list_remove(&view->toplevel_request_fullscreen.link);
		wlr_foreign_toplevel_handle_v1_destroy(view->toplevel_handle);
		view->toplevel_handle = NULL;
	}
	if (view->is_maximized) {
		view->is_maximized = false;
		if (!axis_has_maximized_view(view->server, view)) {
			axis_set_taskbars_visible(view->server, true);
		}
	}
	for (int i = 0; i < 4; i++) {
		if (view->border[i]) {
			wlr_scene_node_destroy(&view->border[i]->node);
			view->border[i] = NULL;
		}
	}
	wl_list_remove(&view->map.link);
	wl_list_remove(&view->unmap.link);
	wl_list_remove(&view->destroy.link);
	wl_list_remove(&view->surface_commit.link);
	wl_list_remove(&view->request_move.link);
	wl_list_remove(&view->request_resize.link);
	wl_list_remove(&view->request_maximize.link);
	wl_list_remove(&view->request_fullscreen.link);
	wl_list_remove(&view->set_title.link);
	wl_list_remove(&view->set_app_id.link);
	free(view);
}

static void xdg_toplevel_request_move(struct wl_listener *listener, void *data) {
	(void)data;
	struct axis_view *view = wl_container_of(listener, view, request_move);
	struct axis_server *server = view->server;
	server->grabbed_view = view;
	server->cursor_mode = AXIS_CURSOR_MOVE;
	server->grab_x = server->cursor->x - view->x;
	server->grab_y = server->cursor->y - view->y;
	axis_view_update_borders(view);
}

static void xdg_toplevel_request_resize(struct wl_listener *listener, void *data) {
	struct axis_view *view = wl_container_of(listener, view, request_resize);
	struct wlr_xdg_toplevel_resize_event *event = data;
	struct axis_server *server = view->server;
	server->grabbed_view = view;
	server->cursor_mode = AXIS_CURSOR_RESIZE;
	server->grab_x = server->cursor->x;
	server->grab_y = server->cursor->y;
	struct wlr_box geo_box;
	wlr_xdg_surface_get_geometry(view->xdg_surface, &geo_box);
	server->grab_geobox = geo_box;
	server->grab_geobox.x += view->x;
	server->grab_geobox.y += view->y;
	server->resize_edges = event->edges;
	axis_view_update_borders(view);
}

static void xdg_toplevel_request_maximize(struct wl_listener *listener, void *data) {
	(void)data;
	struct axis_view *view = wl_container_of(listener, view, request_maximize);
	if (view->is_maximized) {
		axis_wlr_xdg_toplevel_set_size(view->xdg_surface, view->xdg_surface->toplevel, view->saved_geometry.width, view->saved_geometry.height);
		wlr_scene_node_set_position(view->scene_node, view->saved_geometry.x, view->saved_geometry.y);
		axis_wlr_xdg_toplevel_set_maximized(view->xdg_surface, view->xdg_surface->toplevel, false);
		view->is_maximized = false;
		if (view->toplevel_handle) {
			wlr_foreign_toplevel_handle_v1_set_maximized(view->toplevel_handle, false);
		}
		if (!axis_has_maximized_view(view->server, NULL)) {
			axis_set_taskbars_visible(view->server, true);
		}
	} else {
		struct wlr_output *output = wlr_output_layout_get_center_output(view->server->output_layout);
		if (output) {
			view->saved_geometry.x = view->x;
			view->saved_geometry.y = view->y;
			struct wlr_box geo_box;
			wlr_xdg_surface_get_geometry(view->xdg_surface, &geo_box);
			view->saved_geometry.width = geo_box.width;
			view->saved_geometry.height = geo_box.height;

			/* Auto-hide taskbar on maximize to occupy full screen */
			axis_set_taskbars_visible(view->server, false);
			wlr_scene_node_set_position(view->scene_node, 0, 0);
			axis_wlr_xdg_toplevel_set_size(view->xdg_surface, view->xdg_surface->toplevel, output->width, output->height);
			axis_wlr_xdg_toplevel_set_maximized(view->xdg_surface, view->xdg_surface->toplevel, true);
			view->is_maximized = true;
			if (view->toplevel_handle) {
				wlr_foreign_toplevel_handle_v1_set_maximized(view->toplevel_handle, true);
			}
		}
	}
	axis_view_update_borders(view);
}

static void xdg_toplevel_request_fullscreen(struct wl_listener *listener, void *data) {
	(void)data;
	struct axis_view *view = wl_container_of(listener, view, request_fullscreen);
	struct wlr_output *output = wlr_output_layout_get_center_output(view->server->output_layout);
	if (output) {
		if (!view->is_fullscreen) {
			view->saved_geometry.x = view->x;
			view->saved_geometry.y = view->y;
			struct wlr_box geo_box;
			wlr_xdg_surface_get_geometry(view->xdg_surface, &geo_box);
			view->saved_geometry.width = geo_box.width;
			view->saved_geometry.height = geo_box.height;

			wlr_scene_node_set_position(view->scene_node, 0, 0);
			axis_wlr_xdg_toplevel_set_size(view->xdg_surface, view->xdg_surface->toplevel, output->width, output->height);
			axis_wlr_xdg_toplevel_set_fullscreen(view->xdg_surface, view->xdg_surface->toplevel, true);
			view->is_fullscreen = true;
			if (view->toplevel_handle) {
				wlr_foreign_toplevel_handle_v1_set_fullscreen(view->toplevel_handle, true);
			}
		} else {
			axis_wlr_xdg_toplevel_set_size(view->xdg_surface, view->xdg_surface->toplevel, view->saved_geometry.width, view->saved_geometry.height);
			wlr_scene_node_set_position(view->scene_node, view->saved_geometry.x, view->saved_geometry.y);
			axis_wlr_xdg_toplevel_set_fullscreen(view->xdg_surface, view->xdg_surface->toplevel, false);
			view->is_fullscreen = false;
			if (view->toplevel_handle) {
				wlr_foreign_toplevel_handle_v1_set_fullscreen(view->toplevel_handle, false);
			}
		}
	}
	axis_view_update_borders(view);
}

static void xdg_toplevel_surface_commit(struct wl_listener *listener, void *data) {
	(void)data;
	struct axis_view *view = wl_container_of(listener, view, surface_commit);
	axis_view_update_borders(view);
}

static void server_new_xdg_surface(struct wl_listener *listener, void *data) {
	struct axis_server *server = wl_container_of(listener, server, new_xdg_surface);
	struct wlr_xdg_surface *xdg_surface = data;

	if (xdg_surface->role == WLR_XDG_SURFACE_ROLE_POPUP) {
		struct wlr_xdg_surface *parent = wlr_xdg_surface_from_wlr_surface(
			xdg_surface->popup->parent);
#if AXIS_WLR_016_PLUS
		struct wlr_scene_tree *parent_tree = parent->data;
		xdg_surface->data = wlr_scene_xdg_surface_create(
			parent_tree, xdg_surface);
#else
		struct wlr_scene_node *parent_node = parent->data;
		xdg_surface->data = wlr_scene_xdg_surface_create(
			parent_node, xdg_surface);
#endif
		return;
	}
	assert(xdg_surface->role == WLR_XDG_SURFACE_ROLE_TOPLEVEL);

	struct axis_view *view = calloc(1, sizeof(struct axis_view));
	view->server = server;
	view->xdg_surface = xdg_surface;
	view->xdg_toplevel = xdg_surface->toplevel;
#if AXIS_WLR_016_PLUS
	struct wlr_scene_tree *tree = wlr_scene_xdg_surface_create(
			AXIS_SCENE_PARENT(server), xdg_surface);
	view->scene_node = &tree->node;
	view->scene_node->data = view;
	xdg_surface->data = tree;
#else
	view->scene_node = wlr_scene_xdg_surface_create(
			AXIS_SCENE_PARENT(server), xdg_surface);
	view->scene_node->data = view;
	xdg_surface->data = view->scene_node;
#endif

	/* Create 4 border rect nodes (border width 2px) initialized with subtle slate rgba(46, 52, 74, 0.65) */
	for (int i = 0; i < 4; i++) {
		view->border[i] = wlr_scene_rect_create(AXIS_SCENE_PARENT(server), 2, 2, border_color_inactive);
	}

	view->map.notify = xdg_toplevel_map;
#if AXIS_WLR_017_PLUS
	wl_signal_add(&xdg_surface->surface->events.map, &view->map);
#else
	wl_signal_add(&xdg_surface->events.map, &view->map);
#endif
	view->unmap.notify = xdg_toplevel_unmap;
#if AXIS_WLR_017_PLUS
	wl_signal_add(&xdg_surface->surface->events.unmap, &view->unmap);
#else
	wl_signal_add(&xdg_surface->events.unmap, &view->unmap);
#endif
	view->destroy.notify = xdg_toplevel_destroy;
	wl_signal_add(&xdg_surface->events.destroy, &view->destroy);

	view->surface_commit.notify = xdg_toplevel_surface_commit;
	wl_signal_add(&xdg_surface->surface->events.commit, &view->surface_commit);

	struct wlr_xdg_toplevel *toplevel = xdg_surface->toplevel;
	view->request_move.notify = xdg_toplevel_request_move;
	wl_signal_add(&toplevel->events.request_move, &view->request_move);
	view->request_resize.notify = xdg_toplevel_request_resize;
	wl_signal_add(&toplevel->events.request_resize, &view->request_resize);
	view->request_maximize.notify = xdg_toplevel_request_maximize;
	wl_signal_add(&toplevel->events.request_maximize, &view->request_maximize);
	view->request_fullscreen.notify = xdg_toplevel_request_fullscreen;
	wl_signal_add(&toplevel->events.request_fullscreen, &view->request_fullscreen);
	view->set_title.notify = xdg_toplevel_set_title;
	wl_signal_add(&toplevel->events.set_title, &view->set_title);
	view->set_app_id.notify = xdg_toplevel_set_app_id;
	wl_signal_add(&toplevel->events.set_app_id, &view->set_app_id);

	/* Center new windows on screen */
	struct wlr_output *primary_out = wlr_output_layout_get_center_output(server->output_layout);
	if (primary_out) {
		view->x = (primary_out->width - 900) / 2;
		view->y = (primary_out->height - 600) / 2 + 18;
		if (view->x < 40) view->x = 40;
		if (view->y < 40) view->y = 40;
		wlr_scene_node_set_position(view->scene_node, view->x, view->y);
	}
	axis_view_update_borders(view);
}

/* ========================================================================= */
/* Layer Shell (Waybar, Swaybg, Mako, Panels)                                */
/* ========================================================================= */
static void layer_surface_map(struct wl_listener *listener, void *data) {
	(void)data;
	struct axis_layer_surface *surface = wl_container_of(listener, surface, map);
	struct wlr_layer_surface_v1 *wlr_layer = surface->layer_surface;
	struct wlr_output *output = wlr_layer->output;
	if (!output) return;

	int width = wlr_layer->current.actual_width;
	int height = wlr_layer->current.actual_height;
	int x = 0, y = 0;

	if (wlr_layer->current.layer == ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND) {
		x = 0;
		y = 0;
		wlr_scene_node_lower_to_bottom(surface->scene_node);
	} else if (wlr_layer->current.anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP) {
		y = wlr_layer->current.margin.top;
		if (wlr_layer->current.anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT &&
		    !(wlr_layer->current.anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT)) {
			x = output->width - width - wlr_layer->current.margin.right;
		} else {
			x = wlr_layer->current.margin.left;
		}
		wlr_scene_node_raise_to_top(surface->scene_node);
	} else if (wlr_layer->current.anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM) {
		y = output->height - height - wlr_layer->current.margin.bottom;
		x = wlr_layer->current.margin.left;
		wlr_scene_node_raise_to_top(surface->scene_node);
	} else {
		wlr_scene_node_raise_to_top(surface->scene_node);
	}

	surface->x = x;
	surface->y = y;
	wlr_scene_node_set_position(surface->scene_node, x, y);
}

static void layer_surface_unmap(struct wl_listener *listener, void *data) {
	(void)data;
	struct axis_layer_surface *surface = wl_container_of(listener, surface, unmap);
	wlr_scene_node_set_enabled(surface->scene_node, false);
}

static void layer_surface_destroy(struct wl_listener *listener, void *data) {
	(void)data;
	struct axis_layer_surface *surface = wl_container_of(listener, surface, destroy);
	wl_list_remove(&surface->link);
	wl_list_remove(&surface->map.link);
	wl_list_remove(&surface->unmap.link);
	wl_list_remove(&surface->destroy.link);
	wl_list_remove(&surface->surface_commit.link);
	free(surface);
}

static void layer_surface_commit(struct wl_listener *listener, void *data) {
	(void)data;
	struct axis_layer_surface *surface = wl_container_of(listener, surface, surface_commit);
	struct wlr_layer_surface_v1 *wlr_layer = surface->layer_surface;
	struct wlr_output *output = wlr_layer->output;
	if (!output) return;

	uint32_t width = wlr_layer->pending.desired_width;
	uint32_t height = wlr_layer->pending.desired_height;

	if (wlr_layer->pending.layer == ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND) {
		width = (uint32_t)output->width;
		height = (uint32_t)output->height;
	} else {
		if (wlr_layer->pending.anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT &&
		    wlr_layer->pending.anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT) {
			width = (uint32_t)output->width - wlr_layer->pending.margin.left - wlr_layer->pending.margin.right;
		}
		if (wlr_layer->pending.anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP &&
		    wlr_layer->pending.anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM) {
			height = (uint32_t)output->height - wlr_layer->pending.margin.top - wlr_layer->pending.margin.bottom;
		}
		if (height == 0) height = 36;
		if (width == 0) width = (uint32_t)output->width;
	}

	wlr_layer_surface_v1_configure(wlr_layer, width, height);
}

static void server_new_layer_surface(struct wl_listener *listener, void *data) {
	struct axis_server *server = wl_container_of(listener, server, new_layer_surface);
	struct wlr_layer_surface_v1 *wlr_layer = data;

	if (!wlr_layer->output) {
		wlr_layer->output = wlr_output_layout_get_center_output(server->output_layout);
	}
	if (!wlr_layer->output) return;

	struct axis_layer_surface *surface = calloc(1, sizeof(struct axis_layer_surface));
	surface->server = server;
	surface->layer_surface = wlr_layer;

#if AXIS_WLR_016_PLUS
	struct wlr_scene_tree *layer_tree = wlr_scene_subsurface_tree_create(&server->scene->tree, wlr_layer->surface);
	surface->scene_node = &layer_tree->node;
#else
	surface->scene_node = wlr_scene_subsurface_tree_create(&server->scene->node, wlr_layer->surface);
#endif

	surface->map.notify = layer_surface_map;
#if AXIS_WLR_017_PLUS
	wl_signal_add(&wlr_layer->surface->events.map, &surface->map);
#else
	wl_signal_add(&wlr_layer->events.map, &surface->map);
#endif
	surface->unmap.notify = layer_surface_unmap;
#if AXIS_WLR_017_PLUS
	wl_signal_add(&wlr_layer->surface->events.unmap, &surface->unmap);
#else
	wl_signal_add(&wlr_layer->events.unmap, &surface->unmap);
#endif
	surface->destroy.notify = layer_surface_destroy;
	wl_signal_add(&wlr_layer->events.destroy, &surface->destroy);
	surface->surface_commit.notify = layer_surface_commit;
	wl_signal_add(&wlr_layer->surface->events.commit, &surface->surface_commit);

	wl_list_insert(&server->layer_surfaces, &surface->link);

	/* Initial configuration */
	uint32_t width = wlr_layer->pending.desired_width ? wlr_layer->pending.desired_width : (uint32_t)wlr_layer->output->width;
	uint32_t height = wlr_layer->pending.desired_height ? wlr_layer->pending.desired_height : 36;
	if (wlr_layer->pending.layer == ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND) {
		width = (uint32_t)wlr_layer->output->width;
		height = (uint32_t)wlr_layer->output->height;
	}
	wlr_layer_surface_v1_configure(wlr_layer, width, height);
}

/* ========================================================================= */
/* Output Handling (DRM / KMS / Frame Callbacks)                             */
/* ========================================================================= */
static void output_frame(struct wl_listener *listener, void *data) {
	(void)data;
	struct axis_output *output = wl_container_of(listener, output, frame);
	struct wlr_scene *scene = output->server->scene;
	struct wlr_scene_output *scene_output = wlr_scene_get_scene_output(scene, output->wlr_output);
	if (!scene_output) return;

	axis_wlr_scene_output_commit(scene_output);

	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	wlr_scene_output_send_frame_done(scene_output, &now);
}

static void output_destroy(struct wl_listener *listener, void *data) {
	(void)data;
	struct axis_output *output = wl_container_of(listener, output, destroy);
	wl_list_remove(&output->frame.link);
	wl_list_remove(&output->destroy.link);
	wl_list_remove(&output->link);
	free(output);
}

static void server_new_output(struct wl_listener *listener, void *data) {
	struct axis_server *server = wl_container_of(listener, server, new_output);
	struct wlr_output *wlr_output = data;

	wlr_output_init_render(wlr_output, server->allocator, server->renderer);

	/* Determine optimal mode, falling back to 1280x800@60Hz for virtual VM displays without EDID */
	if (!wl_list_empty(&wlr_output->modes)) {
		struct wlr_output_mode *mode = wlr_output_preferred_mode(wlr_output);
		if (!mode) {
			mode = wl_container_of(wlr_output->modes.prev, mode, link);
		}
		if (mode) {
			wlr_output_set_mode(wlr_output, mode);
		}
	} else {
		wlr_output_set_custom_mode(wlr_output, 1280, 800, 60000);
	}

	wlr_output_enable(wlr_output, true);
	if (!wlr_output_commit(wlr_output)) {
		wlr_log(WLR_ERROR, "Failed to commit output mode");
		return;
	}

	struct axis_output *output = calloc(1, sizeof(struct axis_output));
	output->wlr_output = wlr_output;
	output->server = server;
	output->frame.notify = output_frame;
	wl_signal_add(&wlr_output->events.frame, &output->frame);
	output->destroy.notify = output_destroy;
	wl_signal_add(&wlr_output->events.destroy, &output->destroy);
	wl_list_insert(&server->outputs, &output->link);

	wlr_output_layout_add_auto(server->output_layout, wlr_output);

	/* Get or create scene output without creating duplicate entries */
	struct wlr_scene_output *scene_output = wlr_scene_get_scene_output(server->scene, wlr_output);
	if (!scene_output) {
		scene_output = wlr_scene_output_create(server->scene, wlr_output);
	}

	/* Create solid AxisOS One UI Dark (#141826) base rectangle so screen is never blank before clients map */
	float bg_color[4] = { 0x14 / 255.0f, 0x18 / 255.0f, 0x26 / 255.0f, 1.0f };
	struct wlr_scene_rect *bg_rect = wlr_scene_rect_create(AXIS_SCENE_PARENT(server), wlr_output->width, wlr_output->height, bg_color);
	if (bg_rect) {
		wlr_scene_node_lower_to_bottom(&bg_rect->node);
	}

	/* Trigger initial frame render and scanout so monitor does not remain in power-saving / black state */
	if (scene_output) {
		axis_wlr_scene_output_commit(scene_output);
	}

}

/* ========================================================================= */
/* Input Device Management                                                   */
/* ========================================================================= */
static void keyboard_handle_destroy(struct wl_listener *listener, void *data) {
	(void)data;
	struct axis_keyboard *keyboard = wl_container_of(listener, keyboard, destroy);
	wl_list_remove(&keyboard->modifiers.link);
	wl_list_remove(&keyboard->key.link);
	wl_list_remove(&keyboard->destroy.link);
	wl_list_remove(&keyboard->link);
	free(keyboard);
}

static void server_new_keyboard(struct axis_server *server, struct wlr_input_device *device) {
	struct axis_keyboard *keyboard = calloc(1, sizeof(struct axis_keyboard));
	keyboard->server = server;
	keyboard->device = device;

	struct xkb_context *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	struct xkb_rule_names rules = {
		.rules = "",
		.model = "",
		.layout = "us",
		.variant = "",
		.options = ""
	};
	struct xkb_keymap *keymap = xkb_keymap_new_from_names(context, &rules, XKB_KEYMAP_COMPILE_NO_FLAGS);
	if (!keymap) {
		keymap = xkb_keymap_new_from_names(context, NULL, XKB_KEYMAP_COMPILE_NO_FLAGS);
	}
#if AXIS_WLR_017_PLUS
	struct wlr_keyboard *wlr_kb = wlr_keyboard_from_input_device(device);
	wlr_keyboard_set_keymap(wlr_kb, keymap);
	xkb_keymap_unref(keymap);
	xkb_context_unref(context);
	wlr_keyboard_set_repeat_info(wlr_kb, 25, 600);

	keyboard->modifiers.notify = keyboard_handle_modifiers;
	wl_signal_add(&wlr_kb->events.modifiers, &keyboard->modifiers);
	keyboard->key.notify = keyboard_handle_key;
	wl_signal_add(&wlr_kb->events.key, &keyboard->key);
	keyboard->destroy.notify = keyboard_handle_destroy;
	wl_signal_add(&device->events.destroy, &keyboard->destroy);

	wlr_seat_set_keyboard(server->seat, wlr_kb);
#elif AXIS_WLR_016_PLUS
	wlr_keyboard_set_keymap(device->keyboard, keymap);
	xkb_keymap_unref(keymap);
	xkb_context_unref(context);
	wlr_keyboard_set_repeat_info(device->keyboard, 25, 600);

	keyboard->modifiers.notify = keyboard_handle_modifiers;
	wl_signal_add(&device->keyboard->events.modifiers, &keyboard->modifiers);
	keyboard->key.notify = keyboard_handle_key;
	wl_signal_add(&device->keyboard->events.key, &keyboard->key);
	keyboard->destroy.notify = keyboard_handle_destroy;
	wl_signal_add(&device->events.destroy, &keyboard->destroy);

	wlr_seat_set_keyboard(server->seat, device->keyboard);
#else
	wlr_keyboard_set_keymap(device->keyboard, keymap);
	xkb_keymap_unref(keymap);
	xkb_context_unref(context);
	wlr_keyboard_set_repeat_info(device->keyboard, 25, 600);

	keyboard->modifiers.notify = keyboard_handle_modifiers;
	wl_signal_add(&device->keyboard->events.modifiers, &keyboard->modifiers);
	keyboard->key.notify = keyboard_handle_key;
	wl_signal_add(&device->keyboard->events.key, &keyboard->key);
	keyboard->destroy.notify = keyboard_handle_destroy;
	wl_signal_add(&device->events.destroy, &keyboard->destroy);

	wlr_seat_set_keyboard(server->seat, device);
#endif
	wl_list_insert(&server->keyboards, &keyboard->link);
}

static void server_new_pointer(struct axis_server *server, struct wlr_input_device *device) {
	wlr_cursor_attach_input_device(server->cursor, device);

	if (wlr_input_device_is_libinput(device)) {
		struct libinput_device *libinput_dev = wlr_libinput_get_device_handle(device);
		if (libinput_dev) {
			/* Pointer acceleration profile */
			if (libinput_device_config_accel_is_available(libinput_dev)) {
				libinput_device_config_accel_set_profile(libinput_dev, LIBINPUT_CONFIG_ACCEL_PROFILE_ADAPTIVE);
				libinput_device_config_accel_set_speed(libinput_dev, 0.0);
			}
			/* Touchpad natural scrolling */
			if (libinput_device_config_scroll_has_natural_scroll(libinput_dev)) {
				libinput_device_config_scroll_set_natural_scroll_enabled(libinput_dev, 1);
			}
			/* Touchpad tap-to-click */
			if (libinput_device_config_tap_get_finger_count(libinput_dev) > 0) {
				libinput_device_config_tap_set_enabled(libinput_dev, LIBINPUT_CONFIG_TAP_ENABLED);
			}
		}
	}
}

static void server_new_input(struct wl_listener *listener, void *data) {
	struct axis_server *server = wl_container_of(listener, server, new_input);
	struct wlr_input_device *device = data;
	switch (device->type) {
	case WLR_INPUT_DEVICE_KEYBOARD:
		server_new_keyboard(server, device);
		break;
	case WLR_INPUT_DEVICE_POINTER:
	case WLR_INPUT_DEVICE_TOUCH:
	case WLR_INPUT_DEVICE_TABLET_TOOL:
		server_new_pointer(server, device);
		break;
	default:
		break;
	}
	uint32_t caps = WL_SEAT_CAPABILITY_POINTER;
	if (!wl_list_empty(&server->keyboards)) {
		caps |= WL_SEAT_CAPABILITY_KEYBOARD;
	}
	wlr_seat_set_capabilities(server->seat, caps);
}

static void seat_request_cursor(struct wl_listener *listener, void *data) {
	struct axis_server *server = wl_container_of(listener, server, request_cursor);
	struct wlr_seat_pointer_request_set_cursor_event *event = data;
	struct wlr_seat_client *focused_client = server->seat->pointer_state.focused_client;
	if (focused_client == event->seat_client) {
		wlr_cursor_set_surface(server->cursor, event->surface, event->hotspot_x, event->hotspot_y);
	}
}

static void seat_request_set_selection(struct wl_listener *listener, void *data) {
	struct axis_server *server = wl_container_of(listener, server, request_set_selection);
	struct wlr_seat_request_set_selection_event *event = data;
	wlr_seat_set_selection(server->seat, event->source, event->serial);
}

/* ========================================================================= */
/* Main Server Initialization & Event Loop                                   */
/* ========================================================================= */
int main(int argc, char *argv[]) {
	/* Automatically reap spawned child processes to prevent zombies */
	struct sigaction sa_chld = {0};
	sa_chld.sa_handler = SIG_IGN;
	sa_chld.sa_flags = SA_NOCLDWAIT;
	sigaction(SIGCHLD, &sa_chld, NULL);

	wlr_log_init(WLR_DEBUG, NULL);
	char *startup_cmd = NULL;

	int c;
	while ((c = getopt(argc, argv, "s:h")) != -1) {
		switch (c) {
		case 's':
			startup_cmd = optarg;
			break;
		case 'h':
			printf("Usage: %s [-s startup command]\n", argv[0]);
			return 0;
		default:
			return 1;
		}
	}

	struct axis_server server = {0};

	/* 1. Initialize the Wayland Display */
	server.wl_display = wl_display_create();
	if (!server.wl_display) {
		wlr_log(WLR_ERROR, "Failed to create Wayland display");
		return 1;
	}

	/* 2. Initialize the Backend */
#if AXIS_WLR_017_PLUS
	server.backend = wlr_backend_autocreate(server.wl_display, NULL);
#else
	server.backend = wlr_backend_autocreate(server.wl_display);
#endif
	if (!server.backend) {
		wlr_log(WLR_ERROR, "Failed to create wlr_backend");
		wl_display_destroy(server.wl_display);
		return 1;
	}

	/* 3. Set Up Renderer and Allocator */
	if (!getenv("WLR_RENDERER")) {
		/* Auto-detect virtual machine graphics drivers (VirtualBox vmwgfx/vboxvideo, QEMU bochs/qxl/virtio_gpu)
		 * to prevent EGL initialization hangs or crashes on hypervisors without hardware 3D */
		FILE *fmodules = fopen("/proc/modules", "r");
		if (fmodules) {
			char mod_buf[512];
			while (fgets(mod_buf, sizeof(mod_buf), fmodules)) {
				if (strstr(mod_buf, "vmwgfx") || strstr(mod_buf, "vboxvideo") || strstr(mod_buf, "qxl") || strstr(mod_buf, "bochs")) {
					wlr_log(WLR_INFO, "[AxisOS] Hypervisor / virtual graphics detected; defaulting to pixman software renderer");
					setenv("WLR_RENDERER", "pixman", 1);
					setenv("WLR_RENDERER_ALLOW_SOFTWARE", "1", 1);
					setenv("WLR_NO_HARDWARE_CURSORS", "1", 1);
					setenv("LIBGL_ALWAYS_SOFTWARE", "1", 1);
					break;
				}
			}
			fclose(fmodules);
		}
	}

	server.renderer = wlr_renderer_autocreate(server.backend);
	if (!server.renderer) {
		wlr_log(WLR_INFO, "[AxisOS] Default hardware renderer unavailable, retrying with pixman software rendering...");
		setenv("WLR_RENDERER", "pixman", 1);
		server.renderer = wlr_renderer_autocreate(server.backend);
	}
	if (!server.renderer) {
		wlr_log(WLR_ERROR, "Failed to create wlr_renderer");
		wlr_backend_destroy(server.backend);
		wl_display_destroy(server.wl_display);
		return 1;
	}
	wlr_renderer_init_wl_display(server.renderer, server.wl_display);

	server.allocator = wlr_allocator_autocreate(server.backend, server.renderer);
	if (!server.allocator) {
		wlr_log(WLR_ERROR, "Failed to create wlr_allocator");
		wlr_backend_destroy(server.backend);
		wl_display_destroy(server.wl_display);
		return 1;
	}

#if AXIS_WLR_017_PLUS
	wlr_compositor_create(server.wl_display, 5, server.renderer);
#else
	wlr_compositor_create(server.wl_display, server.renderer);
#endif
	wlr_data_device_manager_create(server.wl_display);

	/* 4. Initialize the Shell and Scene Graph */
	server.output_layout = wlr_output_layout_create();
	server.scene = wlr_scene_create();
	wlr_scene_attach_output_layout(server.scene, server.output_layout);

	wl_list_init(&server.views);
#if AXIS_WLR_017_PLUS
	server.xdg_shell = wlr_xdg_shell_create(server.wl_display, 3);
#else
	server.xdg_shell = wlr_xdg_shell_create(server.wl_display);
#endif
	server.new_xdg_surface.notify = server_new_xdg_surface;
	wl_signal_add(&server.xdg_shell->events.new_surface, &server.new_xdg_surface);

	wl_list_init(&server.layer_surfaces);
#if AXIS_WLR_017_PLUS
	server.layer_shell = wlr_layer_shell_v1_create(server.wl_display, 4);
#else
	server.layer_shell = wlr_layer_shell_v1_create(server.wl_display);
#endif
	server.new_layer_surface.notify = server_new_layer_surface;
	wl_signal_add(&server.layer_shell->events.new_surface, &server.new_layer_surface);

	server.foreign_toplevel_mgr = wlr_foreign_toplevel_manager_v1_create(server.wl_display);

	/* 5. Wire Up Hardware Listeners */
	wl_list_init(&server.outputs);
	server.new_output.notify = server_new_output;
	wl_signal_add(&server.backend->events.new_output, &server.new_output);

	server.cursor = wlr_cursor_create();
	wlr_cursor_attach_output_layout(server.cursor, server.output_layout);
	server.cursor_mgr = wlr_xcursor_manager_create(NULL, 24);
	wlr_xcursor_manager_load(server.cursor_mgr, 1);

	server.cursor_motion.notify = server_cursor_motion;
	wl_signal_add(&server.cursor->events.motion, &server.cursor_motion);
	server.cursor_motion_absolute.notify = server_cursor_motion_absolute;
	wl_signal_add(&server.cursor->events.motion_absolute, &server.cursor_motion_absolute);
	server.cursor_button.notify = server_cursor_button;
	wl_signal_add(&server.cursor->events.button, &server.cursor_button);
	server.cursor_axis.notify = server_cursor_axis;
	wl_signal_add(&server.cursor->events.axis, &server.cursor_axis);
	server.cursor_frame.notify = server_cursor_frame;
	wl_signal_add(&server.cursor->events.frame, &server.cursor_frame);

	wl_list_init(&server.keyboards);
	server.new_input.notify = server_new_input;
	wl_signal_add(&server.backend->events.new_input, &server.new_input);

	server.seat = wlr_seat_create(server.wl_display, "seat0");
	server.request_cursor.notify = seat_request_cursor;
	wl_signal_add(&server.seat->events.request_set_cursor, &server.request_cursor);
	server.request_set_selection.notify = seat_request_set_selection;
	wl_signal_add(&server.seat->events.request_set_selection, &server.request_set_selection);

	/* 6. Start the Server */
	const char *socket = wl_display_add_socket_auto(server.wl_display);
	if (!socket) {
		wlr_backend_destroy(server.backend);
		wl_display_destroy(server.wl_display);
		return 1;
	}

	if (!wlr_backend_start(server.backend)) {
		wlr_backend_destroy(server.backend);
		wl_display_destroy(server.wl_display);
		return 1;
	}

	setenv("WAYLAND_DISPLAY", socket, true);
	setenv("GDK_BACKEND", "wayland", true);
	setenv("MOZ_ENABLE_WAYLAND", "1", true);
	setenv("OZONE_PLATFORM", "wayland", true);
	setenv("WLR_RENDERER_ALLOW_SOFTWARE", "1", true);
	setenv("WLR_NO_HARDWARE_CURSORS", "1", true);
	setenv("XDG_CURRENT_DESKTOP", "AxisOS-LiquidNodes", true);
	setenv("XDG_SESSION_TYPE", "wayland", true);
	printf("[AxisOS] Running wlroots Wayland compositor on WAYLAND_DISPLAY=%s\n", socket);
	fflush(stdout);
	wlr_log(WLR_INFO, "[AxisOS] Running wlroots Wayland compositor on WAYLAND_DISPLAY=%s", socket);

	/* Launch background session components */
	if (startup_cmd) {
		spawn_command(startup_cmd);
	}

	/* Run event loop */
	wl_display_run(server.wl_display);

	/* Cleanup on termination */
	wl_display_destroy_clients(server.wl_display);
	wl_display_destroy(server.wl_display);
	return 0;
}
