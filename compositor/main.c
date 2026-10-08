/*
 * AxisOS Native wlroots Wayland Compositor (Horizon Nobara Edition)
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

#include <wayland-server-core.h>
#include <wlr/backend.h>
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
#include <wlr/util/log.h>
#include <xkbcommon/xkbcommon.h>

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
	struct wlr_scene_node *scene_node;
	struct wl_listener map;
	struct wl_listener unmap;
	struct wl_listener destroy;
	struct wl_listener request_move;
	struct wl_listener request_resize;
	struct wl_listener request_maximize;
	struct wl_listener request_fullscreen;
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
};

/* Forward declarations */
static void focus_view(struct axis_view *view, struct wlr_surface *surface);
static void focus_next_view(struct axis_server *server);
static void spawn_command(const char *cmd);

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
			wlr_xdg_toplevel_set_activated(previous, false);
		}
	}
	/* Move view to top of scene */
	wlr_scene_node_raise_to_top(view->scene_node);
	wl_list_remove(&view->link);
	wl_list_insert(&server->views, &view->link);

	if (view->xdg_surface->role == WLR_XDG_SURFACE_ROLE_TOPLEVEL && view->xdg_surface->toplevel) {
		wlr_xdg_toplevel_set_activated(view->xdg_surface, true);
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
		execl("/bin/sh", "/bin/sh", "-c", cmd, (void *)NULL);
		_exit(1);
	}
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
		spawn_command("foot &");
		return true;
	}

	/* Super + D or Super + Space: Nobara App Launcher */
	if (is_super && (sym == XKB_KEY_d || sym == XKB_KEY_D || sym == XKB_KEY_space)) {
		spawn_command("wofi --show drun --prompt 'Search Apps...' &");
		return true;
	}

	/* Super + H: Nobara Welcome & Hardware Driver / Codec Manager */
	if (is_super && (sym == XKB_KEY_h || sym == XKB_KEY_H)) {
		spawn_command("nobara-welcome &");
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
		spawn_command("thunar &");
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
				wlr_xdg_toplevel_send_close(current->xdg_surface);
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
						wlr_xdg_toplevel_set_size(view->xdg_surface, output->width, output->height);
						wlr_xdg_toplevel_set_fullscreen(view->xdg_surface, true);
						view->is_fullscreen = true;
					} else {
						wlr_xdg_toplevel_set_size(view->xdg_surface, view->saved_geometry.width, view->saved_geometry.height);
						wlr_scene_node_set_position(view->scene_node, view->saved_geometry.x, view->saved_geometry.y);
						wlr_xdg_toplevel_set_fullscreen(view->xdg_surface, false);
						view->is_fullscreen = false;
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
					wlr_xdg_toplevel_set_size(view->xdg_surface, view->saved_geometry.width, view->saved_geometry.height);
					wlr_scene_node_set_position(view->scene_node, view->saved_geometry.x, view->saved_geometry.y);
					wlr_xdg_toplevel_set_maximized(view->xdg_surface, false);
					view->is_maximized = false;
				} else {
					struct wlr_output *output = wlr_output_layout_get_center_output(view->server->output_layout);
					if (output) {
						view->saved_geometry.x = view->x;
						view->saved_geometry.y = view->y;
						struct wlr_box geo_box;
						wlr_xdg_surface_get_geometry(view->xdg_surface, &geo_box);
						view->saved_geometry.width = geo_box.width;
						view->saved_geometry.height = geo_box.height;

						/* Reserve 36px at top for Waybar */
						wlr_scene_node_set_position(view->scene_node, 0, 36);
						wlr_xdg_toplevel_set_size(view->xdg_surface, output->width, output->height - 36);
						wlr_xdg_toplevel_set_maximized(view->xdg_surface, true);
						view->is_maximized = true;
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
	wlr_seat_set_keyboard(keyboard->server->seat, keyboard->device);
	wlr_seat_keyboard_notify_modifiers(keyboard->server->seat,
		&keyboard->device->keyboard->modifiers);
}

static void keyboard_handle_key(struct wl_listener *listener, void *data) {
	struct axis_keyboard *keyboard = wl_container_of(listener, keyboard, key);
	struct axis_server *server = keyboard->server;
	struct wlr_event_keyboard_key *event = data;
	struct wlr_seat *seat = server->seat;

	uint32_t keycode = event->keycode + 8;
	const xkb_keysym_t *syms;
	int nsyms = xkb_state_key_get_syms(keyboard->device->keyboard->xkb_state, keycode, &syms);

	bool handled = false;
	uint32_t modifiers = wlr_keyboard_get_modifiers(keyboard->device->keyboard);
	if (event->state == WL_KEYBOARD_KEY_STATE_PRESSED) {
		for (int i = 0; i < nsyms; i++) {
			handled = handle_keybinding(server, modifiers, syms[i]);
			if (handled) break;
		}
	}

	if (!handled) {
		wlr_seat_set_keyboard(seat, keyboard->device);
		wlr_seat_keyboard_notify_key(seat, event->time_msec,
			event->keycode, event->state);
	}
}

/* ========================================================================= */
/* Cursor & Pointer Handling                                                 */
/* ========================================================================= */
static struct axis_view *desktop_view_at(struct axis_server *server,
		double lx, double ly, struct wlr_surface **surface, double *sx, double *sy) {
	struct wlr_scene_node *node = wlr_scene_node_at(&server->scene->node, lx, ly, sx, sy);
	if (node == NULL || node->type != WLR_SCENE_NODE_SURFACE) {
		return NULL;
	}
	*surface = wlr_scene_surface_from_node(node)->surface;
	while (node != NULL && node->data == NULL) {
		node = node->parent;
	}
	return node ? node->data : NULL;
}

static void process_cursor_move(struct axis_server *server) {
	struct axis_view *view = server->grabbed_view;
	view->x = server->cursor->x - server->grab_x;
	view->y = server->cursor->y - server->grab_y;
	wlr_scene_node_set_position(view->scene_node, view->x, view->y);
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
	wlr_xdg_toplevel_set_size(view->xdg_surface, new_width, new_height);
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
		wlr_xcursor_manager_set_cursor_image(server->cursor_mgr, "left_ptr", server->cursor);
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
	struct wlr_event_pointer_motion *event = data;
	wlr_cursor_move(server->cursor, event->device, event->delta_x, event->delta_y);
	process_cursor_motion(server, event->time_msec);
}

static void server_cursor_motion_absolute(struct wl_listener *listener, void *data) {
	struct axis_server *server = wl_container_of(listener, server, cursor_motion_absolute);
	struct wlr_event_pointer_motion_absolute *event = data;
	wlr_cursor_warp_absolute(server->cursor, event->device, event->x, event->y);
	process_cursor_motion(server, event->time_msec);
}

static void server_cursor_button(struct wl_listener *listener, void *data) {
	struct axis_server *server = wl_container_of(listener, server, cursor_button);
	struct wlr_event_pointer_button *event = data;
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
	struct wlr_event_pointer_axis *event = data;
	wlr_seat_pointer_notify_axis(server->seat, event->time_msec, event->orientation,
		event->delta, event->delta_discrete, event->source);
}

static void server_cursor_frame(struct wl_listener *listener, void *data) {
	(void)data;
	struct axis_server *server = wl_container_of(listener, server, cursor_frame);
	wlr_seat_pointer_notify_frame(server->seat);
}

/* ========================================================================= */
/* XDG Surfaces (Application Windows)                                        */
/* ========================================================================= */
static void xdg_toplevel_map(struct wl_listener *listener, void *data) {
	(void)data;
	struct axis_view *view = wl_container_of(listener, view, map);
	wl_list_insert(&view->server->views, &view->link);
	focus_view(view, view->xdg_surface->surface);
}

static void xdg_toplevel_unmap(struct wl_listener *listener, void *data) {
	(void)data;
	struct axis_view *view = wl_container_of(listener, view, unmap);
	wl_list_remove(&view->link);
}

static void xdg_toplevel_destroy(struct wl_listener *listener, void *data) {
	(void)data;
	struct axis_view *view = wl_container_of(listener, view, destroy);
	wl_list_remove(&view->map.link);
	wl_list_remove(&view->unmap.link);
	wl_list_remove(&view->destroy.link);
	wl_list_remove(&view->request_move.link);
	wl_list_remove(&view->request_resize.link);
	wl_list_remove(&view->request_maximize.link);
	wl_list_remove(&view->request_fullscreen.link);
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
}

static void xdg_toplevel_request_maximize(struct wl_listener *listener, void *data) {
	(void)data;
	struct axis_view *view = wl_container_of(listener, view, request_maximize);
	if (view->is_maximized) {
		wlr_xdg_toplevel_set_size(view->xdg_surface, view->saved_geometry.width, view->saved_geometry.height);
		wlr_scene_node_set_position(view->scene_node, view->saved_geometry.x, view->saved_geometry.y);
		wlr_xdg_toplevel_set_maximized(view->xdg_surface, false);
		view->is_maximized = false;
	} else {
		struct wlr_output *output = wlr_output_layout_get_center_output(view->server->output_layout);
		if (output) {
			view->saved_geometry.x = view->x;
			view->saved_geometry.y = view->y;
			struct wlr_box geo_box;
			wlr_xdg_surface_get_geometry(view->xdg_surface, &geo_box);
			view->saved_geometry.width = geo_box.width;
			view->saved_geometry.height = geo_box.height;

			/* Reserve 36px at top for Waybar */
			wlr_scene_node_set_position(view->scene_node, 0, 36);
			wlr_xdg_toplevel_set_size(view->xdg_surface, output->width, output->height - 36);
			wlr_xdg_toplevel_set_maximized(view->xdg_surface, true);
			view->is_maximized = true;
		}
	}
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
			wlr_xdg_toplevel_set_size(view->xdg_surface, output->width, output->height);
			wlr_xdg_toplevel_set_fullscreen(view->xdg_surface, true);
			view->is_fullscreen = true;
		} else {
			wlr_xdg_toplevel_set_size(view->xdg_surface, view->saved_geometry.width, view->saved_geometry.height);
			wlr_scene_node_set_position(view->scene_node, view->saved_geometry.x, view->saved_geometry.y);
			wlr_xdg_toplevel_set_fullscreen(view->xdg_surface, false);
			view->is_fullscreen = false;
		}
	}
}

static void server_new_xdg_surface(struct wl_listener *listener, void *data) {
	struct axis_server *server = wl_container_of(listener, server, new_xdg_surface);
	struct wlr_xdg_surface *xdg_surface = data;

	if (xdg_surface->role == WLR_XDG_SURFACE_ROLE_POPUP) {
		struct wlr_xdg_surface *parent = wlr_xdg_surface_from_wlr_surface(
			xdg_surface->popup->parent);
		struct wlr_scene_node *parent_node = parent->data;
		xdg_surface->data = wlr_scene_xdg_surface_create(
			parent_node, xdg_surface);
		return;
	}
	assert(xdg_surface->role == WLR_XDG_SURFACE_ROLE_TOPLEVEL);

	struct axis_view *view = calloc(1, sizeof(struct axis_view));
	view->server = server;
	view->xdg_surface = xdg_surface;
	view->scene_node = wlr_scene_xdg_surface_create(
			&server->scene->node, xdg_surface);
	view->scene_node->data = view;
	xdg_surface->data = view->scene_node;

	view->map.notify = xdg_toplevel_map;
	wl_signal_add(&xdg_surface->events.map, &view->map);
	view->unmap.notify = xdg_toplevel_unmap;
	wl_signal_add(&xdg_surface->events.unmap, &view->unmap);
	view->destroy.notify = xdg_toplevel_destroy;
	wl_signal_add(&xdg_surface->events.destroy, &view->destroy);

	struct wlr_xdg_toplevel *toplevel = xdg_surface->toplevel;
	view->request_move.notify = xdg_toplevel_request_move;
	wl_signal_add(&toplevel->events.request_move, &view->request_move);
	view->request_resize.notify = xdg_toplevel_request_resize;
	wl_signal_add(&toplevel->events.request_resize, &view->request_resize);
	view->request_maximize.notify = xdg_toplevel_request_maximize;
	wl_signal_add(&toplevel->events.request_maximize, &view->request_maximize);
	view->request_fullscreen.notify = xdg_toplevel_request_fullscreen;
	wl_signal_add(&toplevel->events.request_fullscreen, &view->request_fullscreen);

	/* Center new windows on screen */
	struct wlr_output *primary_out = wlr_output_layout_get_center_output(server->output_layout);
	if (primary_out) {
		view->x = (primary_out->width - 900) / 2;
		view->y = (primary_out->height - 600) / 2 + 18;
		if (view->x < 40) view->x = 40;
		if (view->y < 40) view->y = 40;
		wlr_scene_node_set_position(view->scene_node, view->x, view->y);
	}
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

	surface->scene_node = wlr_scene_subsurface_tree_create(&server->scene->node, wlr_layer->surface);

	surface->map.notify = layer_surface_map;
	wl_signal_add(&wlr_layer->events.map, &surface->map);
	surface->unmap.notify = layer_surface_unmap;
	wl_signal_add(&wlr_layer->events.unmap, &surface->unmap);
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

	wlr_scene_output_commit(scene_output);

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

	if (!wl_list_empty(&wlr_output->modes)) {
		struct wlr_output_mode *mode = wlr_output_preferred_mode(wlr_output);
		wlr_output_set_mode(wlr_output, mode);
		wlr_output_enable(wlr_output, true);
		if (!wlr_output_commit(wlr_output)) {
			return;
		}
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
}

/* ========================================================================= */
/* Input Device Management                                                   */
/* ========================================================================= */
static void server_new_keyboard(struct axis_server *server, struct wlr_input_device *device) {
	struct axis_keyboard *keyboard = calloc(1, sizeof(struct axis_keyboard));
	keyboard->server = server;
	keyboard->device = device;

	struct xkb_context *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	struct xkb_keymap *keymap = xkb_keymap_new_from_names(context, NULL, XKB_KEYMAP_COMPILE_NO_FLAGS);
	wlr_keyboard_set_keymap(device->keyboard, keymap);
	xkb_keymap_unref(keymap);
	xkb_context_unref(context);
	wlr_keyboard_set_repeat_info(device->keyboard, 25, 600);

	keyboard->modifiers.notify = keyboard_handle_modifiers;
	wl_signal_add(&device->keyboard->events.modifiers, &keyboard->modifiers);
	keyboard->key.notify = keyboard_handle_key;
	wl_signal_add(&device->keyboard->events.key, &keyboard->key);

	wlr_seat_set_keyboard(server->seat, device);
	wl_list_insert(&server->keyboards, &keyboard->link);
}

static void server_new_pointer(struct axis_server *server, struct wlr_input_device *device) {
	wlr_cursor_attach_input_device(server->cursor, device);
}

static void server_new_input(struct wl_listener *listener, void *data) {
	struct axis_server *server = wl_container_of(listener, server, new_input);
	struct wlr_input_device *device = data;
	switch (device->type) {
	case WLR_INPUT_DEVICE_KEYBOARD:
		server_new_keyboard(server, device);
		break;
	case WLR_INPUT_DEVICE_POINTER:
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
	server.backend = wlr_backend_autocreate(server.wl_display);
	if (!server.backend) {
		wlr_log(WLR_ERROR, "Failed to create wlr_backend");
		wl_display_destroy(server.wl_display);
		return 1;
	}

	/* 3. Set Up Renderer and Allocator */
	server.renderer = wlr_renderer_autocreate(server.backend);
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

	wlr_compositor_create(server.wl_display, server.renderer);
	wlr_data_device_manager_create(server.wl_display);

	/* 4. Initialize the Shell and Scene Graph */
	server.output_layout = wlr_output_layout_create();
	server.scene = wlr_scene_create();
	wlr_scene_attach_output_layout(server.scene, server.output_layout);

	wl_list_init(&server.views);
	server.xdg_shell = wlr_xdg_shell_create(server.wl_display);
	server.new_xdg_surface.notify = server_new_xdg_surface;
	wl_signal_add(&server.xdg_shell->events.new_surface, &server.new_xdg_surface);

	wl_list_init(&server.layer_surfaces);
	server.layer_shell = wlr_layer_shell_v1_create(server.wl_display);
	server.new_layer_surface.notify = server_new_layer_surface;
	wl_signal_add(&server.layer_shell->events.new_surface, &server.new_layer_surface);

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
