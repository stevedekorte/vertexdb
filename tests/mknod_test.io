
// Check that keys created with mknod and write are equivalent.

base := "http://localhost:8080"

// mknod in root

URL with(base .. "/?action=mknod&key=_key1") fetch
URL with(base .. "/?action=write&key=_key2") post("val")
URL with(base .. "/?action=write&key=_key1") post("val")

// mknod elsewhere

URL with(base .. "/dir/?action=mkdir") fetch

URL with(base .. "/dir/?action=mknod&key=_key1") fetch
URL with(base .. "/dir/?action=write&key=_key2") post("val")
URL with(base .. "/dir/?action=write&key=_key1") post("val")

writeln(URL with(base .. "/?action=select&op=pairs") fetch)