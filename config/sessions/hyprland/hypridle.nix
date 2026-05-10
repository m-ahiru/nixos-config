{
  services.hypridle = {
    enable = true;
    settings = {
      general = {
        lock_cmd = "quickshell -p ~/.config/hypr/scripts/quickshell/Lock.qml";
        before_sleep_cmd = "loginctl lock-session";
        after_sleep_cmd = "hyprctl dispatch dpms on";
      };

      listener = [];
    };
  };
}
