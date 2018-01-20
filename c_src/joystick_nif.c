#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <linux/joystick.h>
#include "erl_nif.h"

#define ASSERT(e) ((void)((e) ? 1 : (assert_error(#e, __func__, __FILE__, __LINE__), 0)))

static void assert_error(const char *expr, const char *func, const char *file, int line)
{
  fflush(stdout);
  fprintf(stderr, "%s:%d:%s() Assertion failed: %s\r\n", file, line, func, expr);
  fflush(stderr);
  abort();
}

typedef struct {
  int fd;
  int open;
  char name[256];
  __u32 version;
  __u8 axes;
  __u8 buttons;
} Joystick;

typedef struct {
  ERL_NIF_TERM atom_ok;
  ERL_NIF_TERM atom_undefined;
  ERL_NIF_TERM atom_error;
  ERL_NIF_TERM atom_nil;
  ERL_NIF_TERM atom_number;
  ERL_NIF_TERM atom_type;
  ERL_NIF_TERM atom_value;
  ERL_NIF_TERM atom_timestamp;
  ERL_NIF_TERM atom_version;
  ERL_NIF_TERM atom_axes;
  ERL_NIF_TERM atom_buttons;
  ERL_NIF_TERM atom_name;

} joystick_priv;

static void js_rt_dtor(ErlNifEnv *env, void *obj)
{
  printf("js_rt_dtor called\r\n");
}

static void js_rt_stop(ErlNifEnv *env, void *obj, int fd, int is_direct_call)
{
  Joystick *rt = (Joystick *)obj;
  printf("js_rt_stop called %s\r\n", (is_direct_call ? "DIRECT" : "LATER"));
}

static void js_rt_down(ErlNifEnv* env, void* obj, ErlNifPid* pid, ErlNifMonitor* joystick)
{
  Joystick *rt = (Joystick *)obj;
  int rv;

  printf("js_rt_down called\r\n");

  if(rt->open) {
    ERL_NIF_TERM undefined;
    enif_make_existing_atom(env, "undefined", &undefined, ERL_NIF_LATIN1);
    rv = enif_select(env, rt->fd, ERL_NIF_SELECT_STOP, rt, NULL, undefined);
    ASSERT(rv >= 0);
  }
}

static ErlNifResourceType *js_rt;
static ErlNifResourceTypeInit js_rt_init = {js_rt_dtor, js_rt_stop, js_rt_down};

static int load(ErlNifEnv* env, void** priv, ERL_NIF_TERM info) {
  joystick_priv* data = enif_alloc(sizeof(joystick_priv));
  if (data == NULL) {
    return 1;
  }

  data->atom_ok = enif_make_atom(env, "ok");
  data->atom_undefined = enif_make_atom(env, "undefined");
  data->atom_error = enif_make_atom(env, "error");
  data->atom_nil = enif_make_atom(env, "nil");
  data->atom_type = enif_make_atom(env, "type");
  data->atom_value = enif_make_atom(env, "value");
  data->atom_number = enif_make_atom(env, "number");
  data->atom_timestamp = enif_make_atom(env, "timestamp");
  data->atom_name = enif_make_atom(env, "name");
  data->atom_buttons = enif_make_atom(env, "buttons");
  data->atom_axes = enif_make_atom(env, "axes");
  data->atom_version = enif_make_atom(env, "version");

  *priv = (void*) data;

  js_rt = enif_open_resource_type_x(env, "monitor", &js_rt_init, ERL_NIF_RT_CREATE, NULL);

  return !js_rt;
}

static int reload(ErlNifEnv* env, void** priv, ERL_NIF_TERM info) {
  return 0;
}

static int upgrade(ErlNifEnv* env, void** priv, void** old_priv, ERL_NIF_TERM info) {
  return load(env, priv, info);
}

static void unload(ErlNifEnv* env, void* priv) {
  enif_free(priv);
}

static ERL_NIF_TERM joystick_start(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
  Joystick *joystick;
  ERL_NIF_TERM res;
  int fd, device_id;
  joystick_priv* priv = enif_priv_data(env);
  ErlNifPid self;

  enif_self(env, &self);
  enif_get_int(env, argv[0], &device_id);
  char buf[15];
  sprintf(buf, "/dev/input/js%d", device_id);
  fd = open(buf, O_NONBLOCK);

  if(fd < 0) {
    return enif_make_tuple2(env, priv->atom_error, enif_make_int(env, fd));
  }
  char name[256];
  __u32 version;
  __u8 axes;
  __u8 buttons;
  ioctl(fd, JSIOCGNAME(256), name);
  ioctl(fd, JSIOCGVERSION, &version);
  ioctl(fd, JSIOCGAXES, &axes);
  ioctl(fd, JSIOCGBUTTONS, &buttons);
  enif_fprintf(stderr, "Opened: %s ", name);
  enif_fprintf(stderr, "Version: 0x%04X ", version);
  enif_fprintf(stderr, "Buttons: %d ", buttons);
  enif_fprintf(stderr, "Axes: %d\r\n", axes);

  joystick = enif_alloc_resource(js_rt, sizeof(Joystick));
  joystick->fd = fd;
  joystick->open = 1;
  strcpy(joystick->name, name);
  joystick->version = version;
  joystick->buttons = buttons;
  joystick->axes = axes;

  enif_monitor_process(env, joystick, &self, NULL);

  res = enif_make_resource(env, joystick);
  enif_release_resource(joystick);

  return enif_make_tuple2(env, priv->atom_ok, res);
}

static ERL_NIF_TERM joystick_stop(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
  Joystick *js;
  joystick_priv* priv = enif_priv_data(env);
  int do_stop;

  if(!enif_get_resource(env, argv[0], js_rt, (void **)&js)) {
    return enif_make_badarg(env);
  }

  do_stop = js->open;
  js->open = 0;

  if(do_stop) {
    printf("Closing fd=%d\r\n", js->fd);
    int rv = enif_select(env, js->fd, ERL_NIF_SELECT_STOP, js, NULL, priv->atom_undefined);
    ASSERT(rv >= 0);
  }

  return priv->atom_ok;
}

static ERL_NIF_TERM joystick_poll(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
  Joystick *js;
  int rv;
  joystick_priv* priv = enif_priv_data(env);

  if(!enif_get_resource(env, argv[0], js_rt, (void **)&js)) {
    return enif_make_badarg(env);
  }

  rv = enif_select(env, js->fd, ERL_NIF_SELECT_READ, js, NULL, priv->atom_undefined);
  ASSERT(rv >= 0);

  return priv->atom_ok;
}

static int map_put(ErlNifEnv *env, ERL_NIF_TERM map_in, ERL_NIF_TERM* map_out, ERL_NIF_TERM key, ERL_NIF_TERM value)
{
  return enif_make_map_put(env, map_in, key, value, map_out);
}

static ERL_NIF_TERM joystick_receive_input(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
  Joystick *joystick;
  joystick_priv* priv = enif_priv_data(env);

  if(!enif_get_resource(env, argv[0], js_rt, (void **)&joystick)) {
    return enif_make_badarg(env);
  }

  struct js_event e;
  int r = read(joystick->fd, &e, sizeof(e));
  if(r > 0) {
    ERL_NIF_TERM map = enif_make_new_map(env);
    map_put(env, map, &map, priv->atom_timestamp, enif_make_int(env, (unsigned int)e.time));
    map_put(env, map, &map, priv->atom_type, enif_make_int(env, (int)e.type));
    if(e.type == JS_EVENT_AXIS) {
      // TODO Make scaling configurable
      long int val = ((int)e.value) / 32.767;
      // enif_fprintf(stderr, "scaling: %d -> %li \r\n", e.number, val);
      map_put(env, map, &map, priv->atom_value, enif_make_long(env, val));
    } else {
      map_put(env, map, &map, priv->atom_value, enif_make_int(env, e.value));
    }
    map_put(env, map, &map, priv->atom_number, enif_make_int(env, (int)e.number));
    return map;
  }
  return enif_make_tuple2(env, priv->atom_error, enif_make_int(env, r));
}

static ERL_NIF_TERM joystick_info(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
  Joystick *joystick;
  joystick_priv* priv = enif_priv_data(env);

  if(!enif_get_resource(env, argv[0], js_rt, (void **)&joystick)) {
    return enif_make_badarg(env);
  }
  ERL_NIF_TERM map = enif_make_new_map(env);
  map_put(env, map, &map, priv->atom_name, enif_make_string(env, joystick->name, ERL_NIF_LATIN1));
  map_put(env, map, &map, priv->atom_version, enif_make_int(env, (int)joystick->version));
  map_put(env, map, &map, priv->atom_axes, enif_make_int(env, (int)joystick->axes));
  map_put(env, map, &map, priv->atom_buttons, enif_make_int(env, (int)joystick->buttons));
  return map;
}

static ErlNifFunc nif_funcs[] = {
  {"start_js", 1, joystick_start},
  {"stop_js", 1, joystick_stop},
  {"poll", 1, joystick_poll},
  {"get_info", 1, joystick_info},
  {"receive_input", 1, joystick_receive_input},
};

ERL_NIF_INIT(Elixir.Joystick, nif_funcs, &load, &reload, &upgrade, &unload)
