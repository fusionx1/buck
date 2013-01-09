This is a Drush extension that creates local sites from variables in drush alias files.

This is a simple as posssible wrapper around Drush commands with a few add-ons. It works only on Ubuntu.For something
more feature-rich and extensible see: http://drupal.org/project/fetcher

Commands
--------

Create or update site:

```drush buck @mysite```

Creates or updates webroot, code, virtual host, /etc/hosts, database and files. It detects if an installation already exists and creates it if it doesn't and updates it if it does. Also fixes permisisons where needed.

Create or update site element:

```drush buck-command [command] @mysite```

Commands include webserver_config, hosts, databaset, code, and files.

Destory site:

```drush buck destroy @mysite```

Types of Sites
--------------

Buck handles two types of sites.

1) Installation profiles

This requires the following in a alias:

'profile' => 'name of profile',
'makefile' => 'link to makefile',

Optional but helpful:

'site-name' => 'Name of site',

2) "Regular" Drupal sites

This requires the following in a drush alias:

'sync-source' => 'name of alias that this site should sync from',

Required Elements
----------------

Buck requires the following elements:

Alias requirements
  + regular site
    -> required
    - uri
    - root
    - sync-source
    
  + profile
    - profile
    - make



