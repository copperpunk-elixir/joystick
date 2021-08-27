#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "erl_nif.h"

typedef struct {
  ERL_NIF_TERM atom_ok;
  ERL_NIF_TERM atom_error;
} joystick_priv;

static int load(ErlNifEnv* env, void** priv, ERL_NIF_TERM info) {
  joystick_priv* data = enif_alloc(sizeof(joystick_priv));
  if (data == NULL) {
    return 1;
  }

  data->atom_ok = enif_make_atom(env, "ok");
  data->atom_error = enif_make_atom(env, "error");

  *priv = (void*) data;

  return 0;
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
  joystick_priv* priv = enif_priv_data(env);

  return enif_make_tuple2(env, priv->atom_error, enif_make_int(env, 1));
}

static ERL_NIF_TERM joystick_stop(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
  return enif_make_badarg(env);
}

static ERL_NIF_TERM joystick_poll(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
  return enif_make_badarg(env);
}

static ERL_NIF_TERM joystick_receive_input(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
  return enif_make_badarg(env);
}

static ERL_NIF_TERM joystick_info(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
  return enif_make_badarg(env);
}

static ErlNifFunc nif_funcs[] = {
  {"start_js", 1, joystick_start, 0},
  {"stop_js", 1, joystick_stop, 0},
  {"poll", 1, joystick_poll, 0},
  {"get_info", 1, joystick_info, 0},
  {"receive_input", 1, joystick_receive_input, 0},
};

ERL_NIF_INIT(Elixir.Joystick, nif_funcs, &load, &reload, &upgrade, &unload)
