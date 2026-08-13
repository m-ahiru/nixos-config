{ config, ... }:
{
    home.file.".config/zen/0qaddgqk.Default Profile/chrome/userChrome.css".source = ./assets/zen-userChrome.css;
    home.file.".config/zen/0qaddgqk.Default Profile/chrome/userContent.css".source = ./assets/zen-userContent.css;
    home.file.".config/zen/0qaddgqk.Default Profile/user.js".text = ''
  user_pref("browser.startup.homepage", "search.mahiru.dev");
  user_pref("browser.newtabpage.enabled", false);
  user_pref("browser.toolbars.bookmarks.visibility", "never");
  user_pref("browser.warnOnQuitShortcut", false);
  user_pref("general.autoScroll", true);
  user_pref("media.eme.enabled", true);
  user_pref("media.videocontrols.picture-in-picture.video-toggle.enabled", false);
  user_pref("permissions.default.webgl", true);
  user_pref("privacy.resistFingerprinting", false);
  user_pref("toolkit.legacyUserProfileCustomizations.stylesheets", true);
  user_pref("webgl.force-enabled", true);
  user_pref("layout.css.prefers-color-scheme.content-override", 0);
'';
}
