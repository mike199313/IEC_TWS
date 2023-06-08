EXTRA_OEMESON += "-Drest=enabled"

PACKAGECONFIG[tftp] = "-Dinsecure-tftp-update=enabled,-Dinsecure-tftp-update=disabled,"
PACKAGECONFIG:append = " tftp"
