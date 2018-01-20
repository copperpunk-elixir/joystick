defmodule Joystick.Mixfile do
  use Mix.Project

  def project do
    [
      app: :joystick,
      version: "0.2.0",
      elixir: "~> 1.5",
      compilers: [:elixir_make] ++ Mix.compilers,
      make_clean: ["clean"],
      make_env: make_env(),
      package: package(),
      description: description(),
      start_permanent: Mix.env == :prod,
      deps: deps()
    ]
  end

  defp make_env() do
    case System.get_env("ERL_EI_INCLUDE_DIR") do
      nil ->
        %{
          "ERL_EI_INCLUDE_DIR" => "#{:code.root_dir()}/usr/include",
          "ERL_EI_LIBDIR" => "#{:code.root_dir()}/usr/lib"
        }
      _ ->
        %{}
    end
  end

  def application do
    [extra_applications: [:logger]]
  end

  defp deps do
    [
      {:elixir_make, "~> 0.4.0", runtime: false},
      {:dialyxir, "~> 0.5.1", only: :dev, runtime: false},
      {:ex_doc, "~> 0.18.1", only: [:dev, :test]},
    ]
  end

  defp package do
    [
      licenses: ["MIT"],
      maintainers: ["konnorrigby@gmail.com"],
      links: %{
        "GitHub" => "https://github.com/connorrigby/joystick",
      },
      files: ["lib", "mix.exs", "README*", "LICENSE*", "c_src", "Makefile"],
      source_url: "https://github.com/connorrigby/joystick"
    ]
  end

  defp description do
    """
    Simple Elixir Joystick Wrapper.
    """
  end
end
