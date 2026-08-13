{ config, ... }:
{
    home.file.".config/zen/0qaddgqk.Default Profile/chrome/userChrome.css".source = ./assets/zen-userChrome.css;
    home.file.".config/zen/0qaddgqk.Default Profile/chrome/userContent.css".source = ./assets/zen-userContent.css;
    home.file.".config/zen/0qaddgqk.Default Profile/user.js".text = ''
// ---- userChrome/userContent laden ----
  user_pref("toolkit.legacyUserProfileCustomizations.stylesheets", true);

  // ---- Transparenz ----
  user_pref("widget.transparent-windows", true);
  user_pref("browser.tabs.allow_transparent_browser", true);
  user_pref("zen.theme.acrylic-elements", true);

  // ---- Start auf search.mahiru.dev ----
  user_pref("browser.startup.page", 1);
  user_pref("browser.startup.homepage", "https://search.mahiru.dev");

  // ---- neuer Tab lädt Seite statt floating urlbar ----
  user_pref("zen.urlbar.replace-newtab", false);

  // ---- keine Tab-Wiederherstellung nach Reboot/Crash ----
  user_pref("browser.sessionstore.resume_from_crash", false);

  // ---- Inspector auf chrome (optional) ----
  user_pref("devtools.chrome.enabled", true);
'';
}
