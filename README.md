[[This]] fetches sites using drush alias in a simples as possible way. For something
more feature-rich see http://drupal.org/project/fetcher

This is not yet ready for use but will be shortly.

Commands:
buck
  + create basic site
    - req:
      - root, uri, sync-source
              (create and pull or rysnc?)
  + create install profile
    - req:
      - root, uri, git, make, profile
buck-update
  + update basic site
    -> db: db_sync (if sync source)
    -> files: rsync (if sync source)
    -> git: git pull (if no sync source)
    - options:
      - backup?
  + update install profile
    -> re-make w/o core
    -> re-install
    - options:
      - backup?
buck-command
  + database
  + dns
  + root
  + webserver_config
  + install

buck-destroy
  + database
  + dns
  + root
  + webserver_config

features
  + backups?
  + revert?
  + base drupal site
  + hooks?
    - database sync
    - file sync
buck-fixperms

Alias requirements
  + regular site
    -> required
    - uri
    - root
    - sync-source
    
  + profile
    - profile
    - make



