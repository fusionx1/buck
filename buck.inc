<?php
/**
 * This is an include file.  These are the "callback" functions that drush
 * uses to get the job done.  See buck.drush.inc for the definition of
 * the commands.
 *
 * Example make file for d7 http://drupal.org/node/852412
 *
 * site-install issues:
 *  - http://drupal.org/node/992928 - Drupal Core
 *  - http://drupal.org/node/983914 - Drush
 * @file
 */

/**
 * Helper function for executing shell commands.  Throws exceptions on error.
 *
 * @param <type> $cmd the command to run
 * @param <type> $throwexception throw exception on error?
 */
function buck_shell_exec($cmd, $throwexception=true) {
  // Note: for shell commands: 0=success, non-zero=error code
  $ret = drush_shell_exec($cmd);
  if ($throwexception && !$ret) {
    $msg = "Command returned unexpected result: $cmd";
    $output = drush_shell_exec_output();
    foreach ($output as $line) {
      $msg="\n  $line";
    }
    throw new Exception($msg);
  }
}

/**
 * Helper function for running SQL on a local mysql database.
 *
 * @param <type> $sql
 */
function buck_mysql_exec($sql, $dbuser, $dbpassword) {
  buck_shell_exec("echo \"$sql\" | mysql -u$dbuser -p$dbpassword");
}

/**
 * Helper function for dereferencing paths.
 *
 * @param <type> $pathin
 * @return <type>
 */
function buck_fixpath($pathin) {
  // fix ~'s
  $homedir = "/home/" . exec('whoami');
  $pathout = str_replace("~", $homedir, $pathin);
  // realpath fails on non-existant folders.  So it's hard to fix .'s and ..'s
  return $pathout;
}

/**
 * Create the DNS.  Edits /etc/hosts on local machine.
 * Entries in file are marked with #buck
 *
 * @param <type> $domain the domain to DNS
 * @param <type> $params
 */
function buck_create_dns($site_record) {
  $domain = $site_record['domain'];
  $file = file_get_contents("/etc/hosts");
  // make sure to test for fully spaces parsed domain in hosts file.
  // otherwise shortened partial domains will throw an exception.
  $domainplus = " " . $site_record['domain'] . " ";
  if (!strpos($file, $domainplus)) {
    drush_log("Creating DNS config (add $domain to /etc/hosts) ...", 'ok');
    buck_shell_exec("echo '127.0.0.1 $domain #' | sudo tee -a /etc/hosts");
    drush_log('... done.', 'ok');
  }
  else {
    drush_log("DNS config $domain already added to /etc/hosts)", 'ok');
  }
}

/**
 * Destroy the DNS.  Edit /etc/hosts file on local machine
 *
 * @param <type> $domain
 * @param <type> $params
 */
function buck_destroy_dns($site_record) {
  $domain = $site_record['domain'];
  buck_shell_exec("sudo sed -i \"/ $domain .*#/d\" /etc/hosts");
}


/**
 * This is the virtual hosts template file for the apache functions.
 */
define('BUCK_APACHE_TEMPLATE', "
<Directory #CODEPATH#>
  Options FollowSymLinks
  AllowOverride None

  RewriteEngine On
  RewriteBase /

  # Rewrite URLs of the form 'index.php?q=x'.
  RewriteCond %{REQUEST_FILENAME} !-f
  RewriteCond %{REQUEST_FILENAME} !-d
  RewriteRule ^(.*)$ index.php?q=$1 [L,QSA]
</Directory>

<VirtualHost *:80>
  ServerName #DOMAIN#
  DocumentRoot #CODEPATH#
  LogLevel warn
  ServerSignature Off
</VirtualHost>
");

/**
 * Configure apache virtualhost file using the template.
 * Files put in /etc/apache2/sites-available/$domain, then a2ensite'd
 * Runs: apache2ctl restart
 *
 * @param <type> $domain
 * @param <type> $params 'codepath'
 */
function buck_create_apache($site_record) {
  // get codepath and make sure it exists.
  $codepath = $site_record['root'];
  $domain = $site_record['domain'];
  if (file_exists("/etc/apache2/sites-available/$domain")) {
    if (!drush_confirm(dt('Vhost ' .  $domain . ' already exists. Do you want to erase its contents?'))) {
      return drush_set_error(0, dt('You did not wish to overwrite the vhost. Aborting.'));
    }
  }

  // get template
  $str_config = BUCK_APACHE_TEMPLATE;

  drush_log("Creating apache config (/etc/apache2/sites-enabled/$domain)", 'ok');

  // make our own :-)
  $str_config = str_replace("#DOMAIN#", $domain, $str_config);
  $str_config = str_replace("#CODEPATH#", $codepath, $str_config);

  $config_handle = fopen("/etc/apache2/sites-available/$domain", "w+");
  fwrite($config_handle, $str_config);

  // enable using script
  buck_shell_exec("sudo a2ensite $domain");

  // restart apache
  buck_shell_exec("sudo apache2ctl restart");

  drush_log('... done.', 'ok');
}

/**
 * Destroy the apache virtual hosts config. a2dissite, then rm file.
 * Runs: apache2ctl restart
 *
 * @param <type> $domain
 * @param <type> $params
 */
function buck_destroy_apache($domain, $params) {
  buck_shell_exec("sudo a2dissite $domain", false);
  $filename = "/etc/apache2/sites-available/$domain";
  if (file_exists($filename)) unlink($filename);  // unlink = delete file
  buck_shell_exec("sudo apache2ctl restart");
}

/**
 * Create the database.  Runs SQL.  Replaces .'s with _'s in domain name
 * to get the database name, database username, and password.
 *
 * @param <type> $domain
 * @param <type> $params
 */
function buck_create_database($domain, $params) {
  $db_spec['database'] = $params['domain_'];
  $db_spec['username'] = $params['dbuser'];
  // FIX
  $db_spec['host'] = 'localhost';
  $db_spec['password'] = $params['dbpassword'];
  // BAD
  $db_spec['driver'] = 'mysql';
  if (drush_sql_db_exists($db_spec)) {
    if (!drush_confirm(dt('Database ' .  $db_spec['database'] . ' already exists. Do you want to erase its contents?'))) {
      return drush_set_error(0, dt('You did not wish to overwrite the databases. Aborting.'));
    }
    _drush_sql_drop($db_spec);
  }
  else {
    _drush_sql_create($db_spec);
  }
}
/**
 * Destroys the database using SQL.
 *
 * @param <type> $domain
 * @param <type> $params
 */
function buck_destroy_database($domain, $params) {
}

function buck_create_root($site_record) {
  $codepath = $site_record['root'];
  $oldpath = $site_record['root'] . '.old';

  // Create or make sure webroot installed.
  if (file_exists($codepath)) {
    if (file_exists($oldpath)) {
      drush_log("Removing existing directory $oldpath", "ok");
      drush_shell_exec("sudo rm -rf $oldpath");
      drush_log("... done", "ok");
    }
    drush_log("Moving existing directory $codepath to $oldpath", "ok");
    buck_shell_exec("mv $codepath $oldpath");
    drush_log("... done", "ok");
  }
  else {
    drush_log("Creating directory $codepath", "ok");
    drush_mkdir($codepath);
    drush_log("... done", "ok");
  }
}

function buck_destroy_root($site_record) {
  $root = $site_record['root'];
  drush_log("Removing existing directory $root", "ok");
  drush_shell_exec("rm -rf $root");
  drush_log("... done", "ok");
}

function buck_get_git($site_record) {
  // Install git.
  if (isset($site_record['git'])) {
    $command = 'git clone ' . $site_record['git'] . ' ' . $site_record['root'];

    drush_log('Downloading code to ' . $site_record['root'] . ' (takes a minute, check network activity) ...', 'ok');
    if (!system($command)) {
      return drush_set_error('DRUSH_PM_GIT_CHECKOUT_PROBLEMS', dt('Unable to clone project.'));
    }
    drush_log("... done", "ok");
  }
  else {
    return drush_set_error('DRUSH_PM_GIT_CHECKOUT_PROBLEMS', dt('Git not found in this site definition. Check your drush alias.'));
  }
}

function buck_profile_site_install($site_record) {
  drush_log('Installing ' . $site_record['profile'] . ' ...', 'ok');

  $opts = array(
    "--root=" . $site_record['root'],
    // TODO: get root user and pass.
    "--db-url=" . 'mysql://root:@localhost/' . $site_record['database'],
    #"--db-prefix=" . $params['db-prefix'],
    "--account-name=admin",
    "--account-pass=admin",
    "-v",
    #"--site-name=" . $params['site-name'],
    #"--site-mail=" . $params['site-mail'],
    #"--sites-subdir=" . $params['sites-subdir'],
  );

  drush_invoke_process(
    drush_sitealias_get_record('@self'),
    'site-install',
    array($site_record['profile']),
    $opts,
    array('#integrate' => TRUE)
  );
  drush_log("... done", "ok");
}

function buck_site_install_make($site_record) {
  if (!isset($site_record['makefile'])) {
    $makefile = $site_record['makefile'];
    // TODO: check url.
    #$makefile = buck_fixpath("$makefile");
    #if (!file_exists($makefile)) throw new Exception("Makefile ". $makefile ." not found.");
    drush_invoke_process(
      drush_sitealias_get_record('@self'),
      'make',
      array($makefile, $codepath),
      array('--prepare-install'),
      array('#integrate' => TRUE)
    );
  }
}

function buck_get_files($site_record) {
  $domain = $site_record['domain'];
  $root = $site_record['root'];
  drush_invoke_process(
    drush_sitealias_get_record('@' . $site_record['site']) ,
    'rsync',
    array('@' . $site_record['site'] . '.' . $site_record['sync_version'] . ':%files ' . '@' . $site_record['site'] . ':%files'),
    array('#integrate' => TRUE)
  );
}
function buck_import_database($site_record) {
  //ssh -t etc.
}
function drush_buck_permissions_cleanup($site_record) {

  $domain = $site_record['domain'];
  $root = $site_record['root'];
  drush_invoke_process(
    drush_sitealias_get_record('@self'),
    'buck-fixperms',
    array(),
    array("--domain=$domain", "--codepath=$root"),
    array('#integrate' => TRUE)
  );
}

/**
 * Deletes the sites files using rm -rf.  Also path.old.
 *
 * @param <type> $domain
 * @param <type> $params 'codepath'
 */
function buck_destroy_code($domain, $params) {
  $codepath = $params['codepath'];
  // only delete a web folder.  "drush qd --codepath=/" would be bad.
  if (!file_exists($codepath . '/index.php')) {
      drush_log('No index.php found.  Aborting deletion of files...', 'warning');
      return;
  }
  buck_shell_exec("sudo rm -rf $codepath");
}

/**
 * Install the site. D6, D7, and D8 versions.
 *
 * @param <type> $domain
 * @param <type> $params
 */
function buck_create_install($domain, $params) {
  // detect the drupal version from the CHANGELOG.txt file.
  $dversion = 'unknown';
  $codepath = $params['codepath'];

  // load make file into string
  // CHANGELOG.txt lives in the core directory in d8. Test for it there first.
  $changelog = $codepath . '/core/CHANGELOG.txt';
  if (!file_exists($changelog)) {
    $changelog = $codepath . '/CHANGELOG.txt';
  }
  $str = file_get_contents($changelog);
  if ( preg_match("/^Drupal 6./m", $str) ) {
    $dversion = 'd6';
  }
  // regex magic.
  if ( preg_match("/^Drupal 7./m", $str) ) {
    $dversion = 'd7';
  }
  if ( preg_match("/^Drupal 8./m", $str) ) {
    $dversion = 'd8';
  }

  // D6 specific
  if ($dversion=='d6') {
    // D6 and D7 have different names for the default profile.
    if ($params['profile']=='standard') {
      $params['profile'] = "default";
    }
    // copy default.settings.php to settings.php and change URL and prefix
    $str = file_get_contents("$codepath/sites/default/default.settings.php");
    $str = str_replace("mysql://username:password@localhost/databasename",
          $params['db-url'],
          $str);
    $str = str_replace("\$db_prefix = ''",
          $params['db-prefix'],
          $str);
    file_put_contents("$codepath/sites/".$params['sites-subdir']."/settings.php",$str);
  }

  // Nuff foreplay, let's install!
  $opts = array(
    "--root=" . $params['codepath'],
    "--db-url=" . $params['db-url'],
    "--db-prefix=" . $params['db-prefix'],
    "--account-name=" . $params['account-name'],
    "--account-pass=" . $params['account-pass'],
    "--account-mail=" . $params['account-mail'],
    "--locale=" . $params['locale'],
    "--clean-url=" . $params['clean-url'],
    "--site-name=" . $params['site-name'],
    "--site-mail=" . $params['site-mail'],
    "--sites-subdir=" . $params['sites-subdir'],
  );

  drush_invoke_process(
    drush_sitealias_get_record('@self'),
    'site-install',
    array($params['profile']),
    $opts,
    array('#integrate' => TRUE)
  );

  // D7 specific
  if ($dversion=='d7') {
    // Create a private files folder.  This is done to make fix-perms not break configuring a private files directory
    if (!file_exists($codepath . '/sites/default/files_private')) {
      buck_shell_exec("sudo mkdir $codepath/sites/default/files_private");
    }
    drush_invoke_process(
      drush_sitealias_get_record('@self'),
      'variable-set',
      array('file_private_path', 'sites/default/files_private'),
      array('--always-set', "--root=$codepath"),
      array('#integrate' => TRUE)
    );
    drush_invoke_process(
      drush_sitealias_get_record('@self'),
      'buck-fixperms',
      array(),
      array("--domain=$domain", "--codepath=$codepath"),
      array('#integrate' => TRUE)
    );
  }

  drush_log("\nURL:  http://".$params['domain']."\nAdmin user:  ".$params['account-name']."\nAdmin password: ".$params['account-pass'], 'ok');
}

/**
 * Return site to situation where "drush qc install --domain=site.dev" could
 * be re-run successfully.
 *
 * @param <type> $domain
 * @param <type> $params 'codepath'
 */
function buck_destroy_install($domain, $params) {
  // only delete a web folder.  "drush qd --codepath=/" would be bad.
  $codepath = $params['codepath'];
  if (!file_exists($codepath . '/index.php')) {
      drush_log('No index.php found.  Aborting un-install of site...', 'warning');
      return;
  }

  // drop and create database and user.
  buck_destroy_database($domain, $params);
  buck_create_database($domain, $params);

  // delete settings.php file
  buck_shell_exec("sudo rm -f $codepath/sites/all/default/settings.php");

  // delete files folder
  buck_shell_exec("sudo rm -rf $codepath/sites/all/default/files");
}
