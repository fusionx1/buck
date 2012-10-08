<?php

include("buck.inc");

/**
 * @file
 *   Buck drush commands.
 *
 *   These commands are designed to be run inside the buck development environment.
 */

/**
 * Simple helper function to keep all the arguments in a list.
 * @return <type>
 */
function _buck_arg_list() {
  return array('code'=>'code', 'database'=>'database', 'apache'=>'apache', 'dns'=>'dns', 'install'=>'install');
}

/**
 * Implements hook_drush_command()
 * @return array
 */
function buck_drush_command() {
  $items = array();
  $items['buck-create'] = array(
    'description' => "Fetch a site.",
    'arguments' => array(
      'site name' => 'Required. Should be the name of the site as it appears in Redmine. Do not include full URL.',
      'version' => 'Optional. Choices are usually live or dev. Defaults to dev. Determines where DB and files come from.',
    ),
    'aliases' => array('bc'),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
  );
  $items['buck-update'] = array(
    'description' => "Update the code, db and files of site.",
    'arguments' => array(
      'site name' => 'Required. Should be the name of the site as it appears in Redmine. Do not include full URL.',
      'version' => 'Optional. Choices are usually live or dev. Defaults to dev. Determines where DB and files come from.',
    ),
    'aliases' => array('bu'),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
  );

  $items['buck-command'] = array(
          'description' => "Create a buck development website (" . implode(', ', _buck_arg_list()) . ").",
          'arguments' => array(
                  'what' => 'Optional. One or more items to create: [' . implode('|', _buck_arg_list()) . '|all] Defaults to "all"',
          ),
          'options' => array(
                  'domain'       => 'Required (all).  Domain name of new site.  Use this to reference site with other commands.  Becomes dns, dbname, dbuser, dbpass, site-name, and kitchen-sink',
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
          'aliases' => array('bcmd'),
          'bootstrap' => DRUSH_BOOTSTRAP_DRUSH, // No bootstrap at all.
  );

  $items['buck-destroy'] = array(
          'description' => "Destroy a buck development website (" . implode(', ', _buck_arg_list()) . ").",
          'arguments' => array(
                  'what' => 'Optional. One or more items to destroy: [' . implode('|', _buck_arg_list()) . '|all] Defaults to "all"',
          ),
          'options' => array(
                  'domain' => 'Domain name to destroy.',
                  'codepath' => 'Path to drupal code.  Defaults to ~/websites/[domain]',
          ),
          'examples' => array(
                  'Simplest format' => 'drush qd --domain=example.dev',
                  'Complete format' => 'drush buck-destroy all --domain=example.dev --codepath=/path/to/drupal',
                  'Remove DNS (/etc/hosts)' => 'drush buck-destroy dns --domain=example.dev',
                  'Remove Apache (/etc/apache2/sites-enabled/)' => 'drush buck-destroy apache --domain=example.dev',
                  'Drop Database (mysql db and user)' => 'drush buck-destroy database --domain=example.dev',
                  'Delete code and files (~/webistes/<domain>)' => 'drush buck-destroy code --domain=example.dev --codepath=/path/to/drupal',
                  'Reset for reinstall (~/webistes/<domain>)' => 'drush buck-destroy install --domain=example.dev --codepath=/path/to/drupal',
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
          'aliases' => array('bf'),
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

/**
 * Helper function to check input parameters
 * @param  $required any parameters that are required
 * @return array  if $ret['fail'] is true, then check failed, and an error was logged.
 */
function _buck_params($required) {
  // check arguments
  $commands = drush_get_arguments();
  $options = drush_get_context('cli');
  $ret = array();

  // Commands

  // what steps do we take: "" -> "all" -> all our setup steps.
  if (count($commands)==1) {
    $commands = array("all");
  }
  if(array_search("all", $commands)!==false) {
    $commands = _buck_arg_list();
  }

  // Options

  // Domain always required
  // remove /'s
  $options['domain'] = str_replace('/', '', $options['domain']);
  if (strlen($options['domain']) > 15) {
    $ret['fail'] = true;
    drush_log("  '--domain' must be less than 16 characters long, for mysql user name to work.", 'error');
  }
  if (strlen($options['domain']) < 3) {
    $ret['fail'] = true;
    drush_log("  '--domain' must be at least 3 characters long.", 'error');
  }
  if (strpos($options['domain'], '.')===false) {
    $ret['fail'] = true;
    drush_log("  '--domain' must include a . for drupal installer to work.", 'error');
  }

  // defaults
  $defaults['domain_'] = $domain_ = str_replace('.','_',$options['domain']);
  // database
  $defaults['dbuser'] = 'root';
  $defaults['dbpassword'] = 'buck';
  // apache, code, install
  $defaults['codepath'] = buck_fixpath("~/websites/" . $options['domain']);
  // code, install (check version)
  $defaults['makefile'] = buck_fixpath("~/websites/default.make");
  // install
  $defaults['profile'] = 'standard';
  $defaults['db-url'] = "mysql://$domain_:$domain_@localhost/$domain_";
  $defaults['db-prefix'] = '';
  $defaults['account-name'] = 'admin';
  $defaults['account-pass'] = 'admin';
  $defaults['account-mail'] = 'admin@'.$options['domain'];
  $defaults['locale'] = 'en';
  $defaults['clean-url'] = '1';
  $defaults['site-name'] = $options['domain'];
  $defaults['site-mail'] = 'info@'.$options['domain'];
  $defaults['sites-subdir'] = 'default';

  // Merge it all together - 2nd array overwrites first.
  $ret = array_merge($ret, $commands);
  $ret = array_merge($ret, $defaults);
  $ret = array_merge($ret, $options);

  // Check required
  foreach($required as $require) {
    if (empty($ret[$require])) {
      $ret['fail'] = true;
      drush_log("  '--$require' is a required option for this command.", 'error');
    }
  }
  return $ret;
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
 * Drush callbackfunction for 'drush buck-create'.
 *
 * This function goes step-by-step in creating the new site.
 *
 * @reurn true on success
 */
function drush_buck_create() {
  drush_print_r('starting here');
  $args = drush_get_arguments();
  $site = $args[1];
  $version = isset($args[2]) ? $arg[2] : 'live';
  $aliases = _drush_sitealias_find_and_load_all_aliases();
  if (!isset($aliases[$site])) {
    return drush_set_error(0, dt($site . ' does not exist. Check your drush aliases.'));
  }
  $site_record[$site] = $aliases[$site];
  _drush_sitealias_add_inherited_values($site_record);
  $site_record = $site_record[$site];
  $site_record['database'] = str_replace('.', '_', $site);

  $site_record['domain'] = isset($site_record['uri']) ? $site_record['uri'] : $site . '.local';

  try {
    buck_create_dns($site_record);
    buck_create_root($site_record);
    // This is an installation profile. It requires code and a site install.
    if (isset($site_record['profile'])) {
      buck_get_git($site_record);
      buck_profile_site_install($site_record);
    }
    // This is a site install from a makefile. Site is installed through make.
    elseif (isset($site_record['makefile'])) {
      buck_site_install_make($site_record);
    }
    // This is a normal site with non-local code, database, and media files.
    else {
      buck_get_git($site_record);
      buck_get_files($site_record);
      buck_import_database($site_record);
    }
    // Add vhost.
    buck_create_apache($site_record);

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
