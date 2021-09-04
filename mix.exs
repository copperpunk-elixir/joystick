defmodule Joystick.Mixfile do
  use Mix.Project

  @version "0.2.2"
  @source_url "https://github.com/copperpunk-elixir/joystick"

  def project do
    [
      app: :joystick,
      version: @version,
      elixir: "~> 1.12",
      compilers: [:elixir_make] ++ Mix.compilers(),
      make_targets: ["all"],
      make_clean: ["clean"],
      package: package(),
      source_url: @source_url,
      docs: docs(),
      description: description(),
      start_permanent: Mix.env() == :prod,
      deps: deps()
    ]
  end

  def application do
    [extra_applications: [:logger]]
  end

  defp deps do
    [
      {:elixir_make, "~> 0.6.2", runtime: false},
      {:dialyxir, "~> 1.1.0", only: :dev, runtime: false},
      {:ex_doc, "~> 0.24", only: :dev, runtime: false}
    ]
  end

  defp package do
    [
      licenses: ["MIT"],
      maintainers: ["greg@copperpunk.com"],
      links: %{
        "GitHub" => @source_url
      },
      files: ["lib", "mix.exs", "README*", "LICENSE*", "c_src", "Makefile"],
    ]
  end

  defp description do
    """
    Simple Elixir Joystick Wrapper.
    """
  end

  defp docs do
    [
      extras: ["README.md"],
      main: "readme",
      source_ref: "v#{@version}",
      source_url: @source_url
    ]
  end
end
