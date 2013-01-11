<?php

/**
 * @file
 *   Buck drush commands.
 */

define('BUCK_WEBROOT', 'webroot');

include_once("buck.inc");

/**
 * Simple helper function to keep all the arguments in a list.
 */
function _buck_create_destroy_arg_list() {
  return array(
    'database' => 'database',
    'dns' => 'dns',
    'root' => 'root',
    'webserver_config' => 'webserver_config',
    'install' => 'install',
  );
}

/**
 * Simple helper function to keep all the arguments in a list.
 */
function _buck_fetch_arg_list() {
  return array(
    'git' => 'git',
    'files' => 'files',
    'database' => 'database',
  );
}

/**
 * Implements hook_drush_command()
 * @return array
 */
function buck_drush_command() {
  $items = array();
  $items['buck'] = array(
    'description' => "Fetch a site.",
    'arguments' => array(
      'drush alias' => 'Required. Drush alias for site, aka @mysite.local.',
      'sync alias' => 'Optional. Drush alias for site we are syncing from. Defaults to sync-source in drush alias. Aka @mysite.dev.',
    ),
    'aliases' => array('bf'),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
  );
  $items['buck-command'] = array(
    'description' => "Create a buck dev element (" . implode(', ', _buck_create_destroy_arg_list()) . ").",
    'arguments' => array(
      'what' => 'Optional. One or more items to create: [' . implode('|', _buck_create_destroy_arg_list()) . '|all] Defaults to "all"',
    ),
    'options' => array(
      'uri'       => 'Required (all).  Domain name of new site.  Use this to reference site with other commands.  Becomes dns, dbname, dbuser, dbpass, site-name, and kitchen-sink',
      'dbuser'       => 'Optional (db).  Root mysql user.  Defaults to "root"',
      'dbpassword'   => 'Optional (db).  Root mysql password.  Defaults to "buck"',
      'codepath'     => 'Optional (apache,code,install).  Path to drupal code.  Defaults to "~/websites/[domain]"',
      'makefile'     => 'Optional (code).  Path to drupal make file.  Defaults to D7 "~/websites/default.make"',
      'profile'      => 'Optional (install).  Name of profile to install.  Defaults to "default" in D6, "standard" in D7',
      'db-url'       => 'Optional (install).  DB url.  Defaults to "mysql://[domain]]:[domain]@localhost/[domain]" (replace . with _ in [domain])',
      'db-prefix'    => 'Optional (install).  DB table prefix.  Defaults to ""',
      'account-name' => 'Optional (install).  Name of user 1.  Defaults to "admin"',
      'account-pass' => 'Optional (install).  Password user 1.  Defaults to "admin"',
      'account-mail' => 'Optional (install).  Email of user 1.  Defaults to "admin@[domain]"',
      'locale'       => 'Optional (install).  Installation profile.  Defaults to "en"',
      'clean-url'    => 'Optional (install).  Enable clean-urls?  Defaults to "1"',
      'site-name'    => 'Optional (install).  Site name.  Defaults to "[domain]"',
      'site-mail'    => 'Optional (install).  Site default email.  Defaults to "info@[domain]"',
      'sites-subdir' => 'Optional (install).  Dir in /sites folder.  Defaults to "default"',
    ),
    'examples' => array(
      'Create D7 site in one command: setup dns, apache, create db, download d6, run installer, open in firefox' => 'drush buck-create --domain=example1.dev',
      'Quick D6 site' => 'drush qc --domain=example2.dev --makefile=d6.make',
      'Just DNS (/etc/hosts)' => 'drush buck-create dns --domain=example.dev',
      'Just apache (/etc/apache2/sites-enabled/)' => 'drush buck-create apache --domain=example.dev --codepath=/path/to/drupal',
      'Just database (mysql)' => 'drush buck-create database --domain=example.dev',
      'Just code (drush make)' => 'drush buck-create code --domain=example.dev',
      'Just install (or quick reinstall)' => 'drush buck-create install --domain=example.dev',
    ),
    'aliases' => array('bcc'),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH, // No bootstrap at all.
  );
  $items['buck-destroy'] = array(
    'description' => "Destroy a buck development website (" . implode(', ', _buck_create_destroy_arg_list()) . ").",
    'arguments' => array(
      'drush alias' => 'Required. Drush alias for site, aka @mysite.local.',
    ),
    'aliases' => array('bd'),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH, // No bootstrap at all.
  );
  $items['buck-fixperms'] = array(
          'description' => "Fix permissions for a buck development website:
  \$ sudo chown -R buck:www-data <codepath>
  \$ sudo chmod -R a=,u=rwX,g=rX <codepath>
  \$ sudo find <codepath>/sites -type d -name files -exec chmod -R a=,ug=rwX '{}' \;
  \$ sudo find <codepath>/sites -type d -name files_private -exec chmod -R a=,ug=rwX '{}' \;",
          'arguments' => array(
          ),
          'options' => array(
                  'domain' => 'Domain name to fix permissions.',
                  'codepath' => 'Path to drupal code.  Defaults to ~/websites/[domain]',
          ),
          'examples' => array(
                  'Simplest format' => 'drush qf --domain=example.dev',
                  'Use different codepath' => 'drush buck-fixperms --domain=example.dev --codepath=/path/to/drupal',
          ),
          'aliases' => array('bp'),
          'bootstrap' => DRUSH_BOOTSTRAP_DRUSH, // No bootstrap at all.
  );
  return $items;
}

/**
 * Implementation of hook_drush_help().
 */
function buck_drush_help($section) {
  switch ($section) {
  }
}

function _buck_required_params($command) {
  $required = array(
    'dns' => array(
      'uri',
    ),
    'database' => array(
      'database',
      'db-url',
    ),
    'root' => array(
      'root',
    ),
    'webserver_config' => array(
      'uri',
      'root',
    ),
    'install' => array(
      'db_user',
      'db_pass',
      //'database',
      'uri',
      'root',
    ),
    'buck' => array(
      'database',
      'db-url',
      'root',
      'uri',
    ),
    'destroy' => array(
      'database',
      'db-url',
      'root',
      'uri',
    ),
  );

  return $required[$command];
}

/**
 * Helper function to check input parameters
 * @param  $required any parameters that are required
 * @return array  if $ret['fail'] is true, then check failed, and an error was logged.
 */
function _buck_check_params($site_record, $required) {
  $ret = array();

  // Should use drush_set_option() for these:
  $domain = str_replace('/', '', $site_record['uri']);
  $database = str_replace(array('.', '-'), '_', $site_record['#name']);
  // DB username
  $defaults['username'] = 'root';
  $defaults['dbpassword'] = '';
  $defaults['database'] = $database;
  $defaults['target-database'] = $database;
  $defaults['profile'] = 'standard';
  $defaults['db-prefix'] = '';
  $defaults['db-su'] = 'root';
  $defaults['db-su-pw'] = '';
  $defaults['account-name'] = 'admin';
  $defaults['account-pass'] = 'admin';
  $defaults['account-mail'] = 'admin@'. $domain;
  $defaults['locale'] = 'en';
  $defaults['clean-url'] = '1';
  $defaults['driver'] = 'mysql';
  $defaults['site-name'] = $domain;
  $defaults['site-mail'] = 'info@'. $domain;
  $defaults['sites-subdir'] = 'default';
  $defaults['webroot'] =  preg_replace('/' . BUCK_WEBROOT  . '$/', '', $site_record['root']);

  // Merge it all together - 2nd array overwrites first.
  $ret = array_merge($ret, $defaults);
  $ret = array_merge($ret, $site_record);
  $site_record = array_merge($site_record, $ret);

  // This will be blank unless added to alias.
  $site_record['db-url'] = 'mysql://' . $site_record['dbuser'] . ':' . $site_record['dbpassword'] . '@localhost/' . $site_record['database'];
  if (strlen($site_record['database']) > 15) {
    $ret['fail'] = true;
    drush_log("  '--uri' must be less than 16 characters long, for mysql user name to work.", 'error');
  }
  if (strlen($site_record['database']) < 3) {
    $ret['fail'] = true;
    drush_log("  '--uri' must be at least 3 characters long.", 'error');
  }

  // Check required
  foreach($required as $require) {
    if (empty($site_record[$require])) {
      $site_record['fail'] = true;
      drush_log("  '--$require' is a required option for this command.", 'error');
      drush_user_abort();
    }
  }
  drush_set_option('@' . $site_record['#name'], $site_record, 'site-aliases');
  return $site_record;
}

/**
 * Drush callbackfunction for 'drush buck-fixperms'.
 *
 * This function fixes permissions for a drupal site.
 *
 * @reurn true on success
 */
function drush_buck_fixperms() {
  $params = _buck_params(array('domain'));
  if (isset($params['fail'])) return drush_set_error(0, dt('Bad parameters.  Try "drush help buck-fixperms"'));

  $codepath = $params['codepath'];
  $current_user = exec('echo "$(whoami)"');

  buck_shell_exec("sudo chown -R $current_user:www-data $codepath");
  buck_shell_exec("sudo chmod -R a=,u=rwX,g=rX $codepath");
  // find directories named "files" and run chmod on them.
  buck_shell_exec("sudo find $codepath/sites -type d -name files -exec chmod -R a=,ug=rwX '{}' \;");  
  buck_shell_exec("sudo find $codepath/sites -type d -name files_private -exec chmod -R a=,ug=rwX '{}' \;");  
}

/**
 * Drush callbackfunction for 'drush buck'.
 *
 * This function goes step-by-step in creating or updating the new site.
 *
 * @return true on success
 */
function drush_buck($environment = NULL) {

  $prepared = buck_env_prepare($environment, 'create');

  $site_record = $prepared['site_record'];

  try {
    buck_create_dns($site_record);
    // This is a site install from a makefile. Site is installed through make.
    if (isset($site_record['makefile'])) {
      // 1) if folder exists
      if (file_exists($site_record['webroot'])) {
        // 2) update
        buck_update_git_profile($site_record);
        buck_update_install_profile($site_record);
      }
      else {
        // 3) create from scratch
        buck_site_install_make($site_record);
        buck_fetch_git_profile($site_record);
        buck_site_install_profile($site_record);
      }
    }
    // This is a normal site with non-local code, database, and media files.
    else {
      buck_create_root($site_record);
      buck_fetch_git($site_record);
      buck_sync_files($site_record);
      buck_sync_database($site_record);
    }
    // Add vhost.
    buck_create_webserver_config($site_record);

    // Cleanup

  } catch (Exception $e) {
    $message = $e->getMessage();
    drush_log("Command failed: $message", "error");
    return false;
  }

  drush_log("Command completed successfully.", "success");
  return true;
}

/**
 * Drush callback function for 'drush buck-destroy'
 *
 * Works step-by-step to destroy the site.
 *
 * @return true on success
 */
function drush_buck_destroy() {

  $params = _buck_params(array('domain'));
  if (isset($params['fail'])) return drush_set_error(0, dt('Bad parameters.  Try "drush help buck-destroy"'));
  $domain = $params['domain'];

  drush_log(' ', 'ok'); // add newline at top of output

  try {
    if (array_search("install", $params)) {
      drush_log('Destroying installation (nothing to do) ...', 'ok');
      // buck_destroy_install($domain, $params);
      drush_log('                                        ... done.', 'ok');
    }
    if (array_search("code", $params)) {
      drush_log('Destroying code at ' . $params['codepath'] . ' ...', 'ok');
      buck_destroy_code($domain, $params);
      drush_log('                                        ... done.', 'ok');
    }
    if (array_search("database", $params)) {
      drush_log('Destroying database and user ' . $params['domain_'] . ' ...', 'ok');
      buck_destroy_database($domain, $params);
      drush_log('                                        ... done.', 'ok');
    }
    if (array_search("apache", $params)) {
      drush_log("Destroying apache config (/etc/apache2/sites-enabled/$domain) ...", 'ok');
      buck_destroy_apache($domain, $params);
      drush_log('                                        ... done.', 'ok');
    }
    if (array_search("dns", $params)) {
      drush_log("Destroying dns config (remove $domain from /etc/hosts) ...", 'ok');
      buck_destroy_dns($domain, $params);
      drush_log('                                        ... done.', 'ok');
    }
  } catch (Exception $e) {
    $message = $e->getMessage();
    drush_log("Command failed: $message", "error");
    return false;
  }

  drush_log("Command completed successfully.", "success");
  return true;
}

/**
 * Drush callbackfunction for 'drush buck-command-create'.
 *
 * @reurn true on success
 */
function drush_buck_command($environment = NULL) {
  $prepared = buck_env_prepare($environment, 'command_create');
  call_user_func('buck_create_' . $prepared['command'], $prepared['site_record']); 
}

/**
 * Drush callbackfunction for 'drush buck-command-destroy'.
 *
 * @reurn true on success
 */
function drush_buck_command_destroy($environment = NULL) {
  $prepared = buck_env_prepare($environment, 'destroy');
  call_user_func('buck_destroy_' . $prepared['command'], $prepared['site_record']); 
}

/**
 * Gets alias info and overwrites with added options.
 * Checks that alias exists and enough info is supplied.
 */
function buck_env_prepare($environment) {
  // Get alias information.
  $env = buck_get_env($environment);

  // Get allowed arguments.
  $cmds = array_merge(_buck_create_destroy_arg_list(), _buck_fetch_arg_list());
  // Get actual arguments.
  $args = drush_get_arguments();
  // Grab command from arguments.
  $buck_command = array_shift($args);
  // Get options.
  $options = drush_get_context('cli');
  // Add sync-source to options if supplied.
  if (count($args > 1) && drush_sitealias_get_record($args[1])) {
    $options['sync-source'] = $args[1];
  }

  // Options can override alias info.
  if ($env) {
    $site_record = array_merge($options, $env);
  }
  else {
    $site_record = $options;
  }

  $required = _buck_required_params($buck_command);

  $site_record = _buck_check_params($site_record, $required);

  $prepared['site_record'] = $site_record;
  $prepared['command'] = $buck_command;
  return $prepared;
}
