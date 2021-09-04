defmodule Joystick do
  @moduledoc """
  Simple wrapper to get Linux Joystick events.

  # Usage
  ```
  iex()> {:ok, js} = Joystick.start_link(0, self())
  iex()> flush()
  {:joystick, %Joystick.Event{number: 1, timestamp: 1441087318, type: :axis, value: -60}}
  {:joystick, %Joystick.Event{number: 4, timestamp: 1441087318, type: :axis, value: -5}}
  iex()> Joystick.info(js)
  %{axes: 8, buttons: 11, name: 'Microsoft X-Box One pad', version: 131328}
  ```
  """

  use GenServer
  require Logger

  defmodule Event do
    @moduledoc false
    defstruct [:number, :timestamp, :type, :value]

    @compile {:inline, decode: 1}
    @doc false
    def decode(%{timestamp: _, number: _, type: 0x01, value: _} = data) do
      struct(__MODULE__, %{data | type: :button})
    end

    def decode(%{timestamp: _, number: _, type: 0x02, value: _} = data) do
      struct(__MODULE__, %{data | type: :axis})
    end

    def decode(%{timestamp: _, number: _, type: 0x81, value: _} = data) do
      # Init Button
      struct(__MODULE__, %{data | type: :button})
    end

    def decode(%{timestamp: _, number: _, type: 0x82, value: raw_value} = data) do
      # Init Axis
      # must scale axis value
      value_scaled = raw_value / 32.767

      value =
        cond do
          value_scaled > 999 -> 999
          value_scaled < -999 -> -999
          true -> value_scaled
        end

      struct(__MODULE__, %{data | type: :axis, value: value})
    end
  end

  @doc """
  Start listening to joystick events.
  * `device` - a number pointing to the js file.
    * for example 0 would evaluate to "/dev/input/js0"
  * `listener` - pid to receive events
  * `callback` - a message will be sent to `listener` with the form `{callback, joystick}`, where
  `joystick` is be the pid of the Joystick GenServer. Default value of `callback` is `nil`, which means the
  `Joystick` pid will be returned immediately, but the GenServer will crash if a joystick connection cannot be opened.
  """
  def start_link(device, listener, callback \\ nil) do
    GenServer.start_link(__MODULE__, [device, listener, callback])
  end

  @doc "Get information about a joystick"
  def info(joystick) do
    GenServer.call(joystick, :info)
  end

  @doc """
  Stop a running joystick instance.
  """
  def stop(joystick, reason \\ :normal) do
    GenServer.stop(joystick, reason)
  end

  @doc false
  def init([device, listener, callback]) do
    state =
      if is_nil(callback) do
        {:ok, res} = start_js(device)
        js = get_info(res)
        :ok = poll(res)

        %{res: res, listener: listener, last_ts: 0, joystick: js}
      else
        GenServer.cast(self(), {:connect_to_joystick, device, listener, callback})
        %{joystick: nil}
      end

    {:ok, state}
  end

  def handle_cast({:connect_to_joystick, device, listener, callback}, state) do
    Logger.debug("Joystick attempting to connect to #{device}")

    state =
      case start_js(device) do
        {:ok, res} ->
          js = get_info(res)
          :ok = poll(res)

          GenServer.cast(listener, {callback, js})
          %{res: res, listener: listener, last_ts: 0, joystick: js}

        {:error, error} ->
          Logger.warn("Joystick could not connect: #{inspect(error)}")
          Logger.warn("Retrying in 1000ms.")
          Process.sleep(1000)
          GenServer.cast(self(), {:connect_to_joystick, device, listener})
          state

        other ->
          raise "Joystick should not have reached here: #{inspect(other)}"
      end

    {:noreply, state}
  end

  @doc false
  def terminate(_, state) do
    if state.res do
      stop_js(state.res)
    end
  end

  @doc false
  def handle_call(:info, _, state), do: {:reply, state.joystick, state}

  @doc false
  def handle_info({:select, res, _ref, :ready_input}, %{last_ts: last_ts} = state) do
    {time, raw_input} = :timer.tc(fn -> Joystick.receive_input(res) end)

    case raw_input do
      {:error, reason} ->
        {:stop, {:input_error, reason}, state}

      input = %{timestamp: current_ts} when current_ts >= last_ts ->
        event = {:joystick, Event.decode(input)}
        send(state.listener, event)
        :ok = poll(res)
        # Logger.debug "Event (#{time}µs): #{inspect event}"
        {:noreply, %{state | last_ts: current_ts}}

      event = %{timestamp: current_ts} ->
        Logger.warn("Got late event (#{time}µs): #{inspect(event)}")
        :ok = poll(res)
        {:noreply, %{state | last_ts: current_ts}}
    end
  end

  @on_load :load_nif
  @doc false
  def load_nif do
    # _nif'
    nif_file = '#{:code.priv_dir(:joystick)}/joystick'

    case :erlang.load_nif(nif_file, 0) do
      :ok -> :ok
      {:error, {:reload, _}} -> :ok
      {:error, reason} -> Logger.warn("Failed to load nif: #{inspect(reason)}")
    end
  end

  ## These functions get replaced by the nif.
  @doc false
  def start_js(_device), do: do_exit_no_nif()
  @doc false
  def stop_js(_handle), do: do_exit_no_nif()
  @doc false
  def poll(_handle), do: do_exit_no_nif()
  @doc false
  def receive_input(_handle), do: do_exit_no_nif()
  @doc false
  def get_info(_handle), do: do_exit_no_nif()

  ## Private stuff

  defp do_exit_no_nif, do: exit("nif not loaded.")
end
