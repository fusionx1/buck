<?php
/**
 * @file
 * This is an include file.  These are the "callback" functions that drush
 * uses to get the job done.  See buck.drush.inc for the definition of
 * the commands.
 */

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
 * This is the virtual hosts template file for settings.php file.
 */
define('BUCK_SETTINGS_PHP_TEMPLATE', "
<?php
\$databases = array (
  'default' =>
  array (
    'default' =>
    array (
      'database' => '#DATABASE#',
      'username' => '#USERNAME#',
      'password' => '#PASSWORD#',
      'host' => 'localhost',
      'port' => '',
      'driver' => 'mysql',
      'prefix' => '',
    ),
  ),
);
\$base_url = '#URL#';
ini_set('session.gc_probability', 1);
ini_set('session.gc_divisor', 100);
ini_set('session.gc_maxlifetime', 200000);
ini_set('session.cookie_lifetime', 2000000);
");

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
  $domain = $site_record['uri'];
  $file = file_get_contents("/etc/hosts");
  // make sure to test for fully spaces parsed domain in hosts file.
  // otherwise shortened partial domains will throw an exception.
  $domainplus = " " . $site_record['uri'] . " ";
  if (!strpos($file, $domainplus)) {
    drush_log("Creating DNS config (add $domain to /etc/hosts) ...", 'ok');
    buck_shell_exec("echo '127.0.0.1 $domain #' | sudo tee -a /etc/hosts");
    drush_log('... done.', 'ok');
  }
  else {
    drush_log("DNS config $domain already added to /etc/hosts", 'ok');
  }
}

/**
 * Destroy the DNS.  Edit /etc/hosts file on local machine
 *
 * @param <type> $domain
 * @param <type> $params
 */
function buck_destroy_dns($site_record) {
  $domain = $site_record['uri'];
  buck_shell_exec("sudo sed -i \"/ $domain .*#/d\" /etc/hosts");
}

/**
 * Configure apache virtualhost file using the template.
 */
function buck_create_webserver_config($site_record) {
  $domain = $site_record['uri'];
  if (file_exists("/etc/apache2/sites-available/$domain")) {
    if (!drush_confirm(dt('Vhost ' .  $domain . ' already exists. Do you want to erase its contents?'))) {
      return drush_set_error(0, dt('You did not wish to overwrite the vhost. Aborting.'));
    }
  }

  $str_config = BUCK_APACHE_TEMPLATE;

  drush_log("Creating apache config (/etc/apache2/sites-enabled/$domain)", 'ok');

  $str_config = str_replace("#DOMAIN#", $domain, $str_config);
  $str_config = str_replace("#CODEPATH#", $site_record['root'], $str_config);

  $config_handle = fopen("/etc/apache2/sites-available/$domain", "w+");
  fwrite($config_handle, $str_config);

  buck_shell_exec("sudo a2ensite $domain");

  // restart apache
  buck_shell_exec("sudo /etc/init.d/apache2 restart");

  drush_log('... done.', 'ok');
}

/**
 * Destroy the apache virtual hosts config. a2dissite, then rm file.
 */
function buck_destroy_webserver_config($site_record) {
  $domain = $site_record['uri'];
  buck_shell_exec("sudo a2dissite $domain", false);
  $filename = "/etc/apache2/sites-available/$domain";
  if (file_exists($filename)) unlink($filename);  // unlink = delete file
  drush_log("Removing apache config (/etc/apache2/sites-enabled/$domain)", 'ok');
  buck_shell_exec("sudo /etc/init.d/apache2 restart");
}

/**
 * Destroys the database using SQL.
 */
function buck_destroy_database($site_record) {
  // There is no sql-destroy in drush.
  drush_shell_exec('mysql -e "drop database ' . $site_record['database'] . '"');
}

function buck_create_root($site_record) {
  $codepath = $site_record['webroot'];
  $tar_file = $site_record['database'] . '-' . date('y-m-d-h') . '.tar.gz';
  $tar_dir = '/var/www/' . trim(strrchr($site_record['webroot'], '/'), '/');

  // Create or make sure webroot installed.
  if (file_exists($codepath)) {
    if (!drush_confirm(dt('Folder ' . $site_record['webroot'] . ' already exists. Do you want to remove it?'))) {
      return;
    }
    if (file_exists($oldpath)) {
      drush_log("Removing existing directory $oldpath", "ok");
      drush_shell_exec("rm -rf $oldpath");
      drush_log("... done", "ok");
    }
    drush_log("Archiving existing directory $codepath to $tar_file", "ok");
    buck_shell_exec("/bin/tar zcfP $tar_file $codepath");
    drush_log("Removing existing directory $codepath", "ok");
    drush_delete_dir("$codepath");
    drush_log("... done", "ok");
    drush_log("Creating directory $codepath", "ok");
    drush_mkdir($site_record['webroot']);
    drush_log("... done", "ok");
  }
  else {
    drush_log("Creating directory $codepath", "ok");
    drush_mkdir($site_record['webroot']);
    drush_log("... done", "ok");
  }
}

function buck_destroy_root($site_record) {
  $root = $site_record['root'];
  drush_log("Removing existing directory $root", "ok");
  drush_shell_exec("rm -rf $root");
  drush_log("... done", "ok");
}

/**
 * Updates existing profile managed by git.
 */
function buck_update_git_profile($site_record) {
  $profile_dir = $site_record['root'] . '/profiles/' . $site_record['profile'];

  $command = 'git pull --rebase';
  drush_shell_cd_and_exec($site_record['root'], 'git pull --rebase');

  $git_ignore = $profile_dir . '/.gitignore';
  if (file_exists($git_ignore)) {
    $profile_ignored = file($git_ignore);

    foreach ($profile_ignored as $dir) {
      drush_delete_dir($profile_dir . '/' . trim($dir));
    }
  }
  else {
    return drush_set_error('DRUSH_PM_GIT_CHECKOUT_PROBLEMS', dt('Could not find .gitignore file.'));
  }
  $tmp_dir = drush_tempdir() . '/' . $site_record['profile'];
  $make = drush_invoke_process(
    '@self',
    'make',
    array($site_record['makefile'], $tmp_dir),
    array('--no-core')
  );

  foreach ($profile_ignored as $dir) {
    $ignore_dir = $tmp_dir . "/profiles/" . $site_record['profile'] . '/' . trim($dir);
    drush_move_dir($ignore_dir, $profile_dir . '/' . trim($dir), TRUE);
  }
}

/**
 * Moves existing profile to temp directory, installs git version, moves all
 * ignored folders back to profile.
 * Finished product is a profile that is version controlled by git.
 */
function buck_fetch_git_profile($site_record) {
  $profile_dir = $site_record['root'] . '/profiles/' . $site_record['profile'];

  // Move profile into temp directory.
  $tmp_dir = drush_tempdir();
  drush_move_dir($profile_dir, $tmp_dir, TRUE);
  drush_mkdir($profile_dir);

  // Install git profile.
  $branch = isset($site_record['git']['branch']) ? '--branch ' . $site_record['git']['branch'] : '';
  $command = 'git clone ' . $branch . ' ' . $site_record['git']['url'] . ' ' . $profile_dir;
  drush_log('Downloading profile code to ' . $profile_dir . ' (takes a minute, check network activity) ...', 'ok');
  buck_shell_exec($command);
  drush_log("... done", "ok");

  // Move folders created during make back into profile.
  $git_ignore = $profile_dir . '/.gitignore';
  if (file_exists($git_ignore)) {
    $profile_ignored = file($git_ignore);

    foreach ($profile_ignored as $dir) {
      $ignore_dir = $tmp_dir . "/" . trim($dir);
      drush_move_dir($ignore_dir, $profile_dir . '/' . trim($dir), TRUE);
    }
  }
  else {
    return drush_set_error('DRUSH_PM_GIT_CHECKOUT_PROBLEMS', dt('Could not find .gitignore file.'));
  }

}

/**
 * Sync files from remote to local.
 */
function buck_sync_files($site_record) {
  // TODO: Add hook for hosts like pantheon that don't support drush rsync.
  drush_invoke_process(
    '@self',
    'rsync',
    array("@" . $site_record['sync-source'] . ":%files", "@" . $site_record['#name'] . ":%files"),
    array("-v"),
    array('interactive' => TRUE)
  );
  drush_log("... done", "ok");
}

/**
 * Clone or pull a git repo.
 */
function buck_fetch_git($site_record) {
  // Install git.
  if (isset($site_record['git'])) {
    if (file_exists($site_record['root'] . '/.git')) {
      drush_log('Git repo at ' . $site_record['root'] . '/.git already exists. Updating...', 'ok');
      drush_shell_cd_and_exec($site_record['root'], 'git pull --rebase');
      return ;
    }

    $branch = isset($site_record['git']['branch']) ? '--branch ' . $site_record['git']['branch'] : '';
    $command = 'git clone ' . $branch . ' ' . $site_record['git']['url'] . ' ' . $site_record['root'];
    drush_log('Downloading code to ' . $site_record['broot'] . ' (takes a minute, check network activity) ...', 'ok');
    if (!system($command)) {
      return drush_set_error('DRUSH_PM_GIT_CHECKOUT_PROBLEMS', dt('Unable to clone project.'));
    }
    drush_log("... done", "ok");
  }
  else {
    return drush_set_error('DRUSH_PM_GIT_CHECKOUT_PROBLEMS', dt('Git not found in this site definition. Check your drush alias.'));
  }
}

/**
 * Re-installs site using profile.
 */
function buck_update_install_profile($site_record) {
  // TODO: Need to check settings.php files.
  $user = posix_getpwuid(posix_geteuid());
  $db_super = parse_ini_file($user['dir'] . '/.my.cnf');
  if (!$db_super['user']) {
    drush_set_error("Your .my.cnf file is not setup correctly.");
    die();
  }
  $opts = array(
    "--account-name=" . $site_record['account-name'],
    "--account-pass=" . $site_record['account-pass'],
    "--site-name=" . $site_record['site-name'],
    "--db-su=" . $db_super['user'],
    "--db-su-pw=" . $db_super['password'],
  );
  drush_invoke_process(
    drush_sitealias_get_record('@' . $site_record['#name']),
    'site-install',
    array($site_record['profile']),
    array($opts),
    array('#integrate' => TRUE)
  );
}

/**
 * Installs site using profile.
 */
function buck_site_install_profile($site_record) {
  drush_log('Installing ' . $site_record['profile'] . ' ...', 'ok');

  $db_pass = drush_generate_password();
  $user = posix_getpwuid(posix_geteuid());
  $db_super = parse_ini_file($user['dir'] . '/.my.cnf');
  if (!$db_super['user']) {
    drush_set_error("Your .my.cnf file is not setup correctly.");
    die();
  }
  $opts = array(
    "--root=" . $site_record['root'],
    "--db-url=" . 'mysql://' . $site_record['#name'] . ':' . $db_pass . '@localhost/' . $site_record['database'],
    "-v",
    "--db-su=" . $db_super['user'],
    "--db-su-pw=" . $db_super['password'],
    "--account-name=" . $site_record['account-name'],
    "--account-pass=" . $site_record['account-pass'],
    "--site-name=" . $site_record['site-name'],
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

/**
 * Makes site via make file.
 */
function buck_site_install_make($site_record) {
  if (isset($site_record['makefile'])) {

    if (file_exists($site_record['profile'])) {
      $make = drush_invoke_process(
        '@self',
        'make',
        array($site_record['makefile'], $site_record['root']),
        array('--no-core')
      );
    }
    else {
      buck_create_root($site_record);
    }

    // TODO: check url.
    #$makefile = buck_fixpath("$makefile");
    #if (!file_exists($makefile)) throw new Exception("Makefile ". $makefile ." not found.");
    $make = drush_invoke_process(
      '@self',
      'make',
      array($site_record['makefile'], $site_record['root']),
      array('--prepare-install')
    );
  }
}

/**
 * Syncs your database from a remote database.
 */
function buck_sync_database($site_record) {
  // TODO: Add hook for hosts like pantheon that don't support drush sql-sync.

  // If there is no local database, need to create one and add it to settings.php
  if (!drush_sql_db_exists($site_record) || !file_exists($site_record['root'] . '/sites/default/settings.php')) {
    // Create database.
    $db_pass = drush_generate_password();
    $db_url = 'mysql://' . $site_record['database'] . ':' . $db_pass  . '@localhost/' . $site_record['database'];

    $user = posix_getpwuid(posix_geteuid());
    $db_super = parse_ini_file($user['dir'] . '/.my.cnf');
    if (!$db_super['user']) {
      drush_set_error("Your .my.cnf file is not setup correctly.");
      die();
    }
    drush_invoke_process(
      '@self',
      'sql-create',
      array("@" . $site_record['sync-source'], "@" . $site_record['#name']),
      array('--db-url=' . $db_url, '--db-su=' . $db_super['user'], '--db-su-pw=' . $db_super['password']),
      array('interactive' => TRUE)
    );

    // Add settings.php
    $str_config = BUCK_SETTINGS_PHP_TEMPLATE;
    $str_config = str_replace("#DATABASE#", $site_record['database'], $str_config);
    $str_config = str_replace("#USERNAME#", $site_record['database'], $str_config);
    $str_config = str_replace("#PASSWORD#", $db_pass, $str_config);
    $str_config = str_replace("#URL#", 'http://' . $site_record['uri'], $str_config);

    $config_handle = fopen($site_record['root'] . "/sites/default/settings.php", "w+");
    fwrite($config_handle, $str_config);
  }
  drush_invoke_process(
    '@self',
    'sql-sync',
    array("@" . $site_record['sync-source'], "@" . $site_record['#name']),
    array('--target-databsae=' . $site_record['database'],  '--db-su=' . $db_super['user'], '--db-su-pw=' . $db_super['password']),
    array('interactive' => TRUE)
  );
}

/**
 * TODO: FIX!
 */
function drush_buck_permissions_cleanup($site_record) {

  $domain = $site_record['domain'];
  $root = $site_record['root'];
  drush_invoke_process(
    '@self',
    'buck-fixperms',
    array(),
    array("--domain=$domain", "--codepath=$root"),
    array('#integrate' => TRUE)
  );
}

/**
 * Deletes the sites files using rm -rf.  Also path.old.
 */
function buck_destroy_code($site_record) {
  $codepath = $site_record['webroot'];
  buck_shell_exec("rm -rf $codepath");
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
    "--root=" . $params['webroot'],
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
      buck_shell_exec("mkdir $codepath/sites/default/files_private");
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

function buck_get_env($environment) {
  if (!$environment) {
    return drush_set_error(dt('You must specify a drush alias with the buck command.'));
  }
  $env = drush_sitealias_get_record($environment);
  if (!$env) {
    // OK, we aren't calling this outside webroot, see if we are in a site directory.
    $env = drush_sitealias_get_record("@self");
    if (!$env) {
      return drush_set_error(dt('Failed to load site alias for !name', array('!name' => $environment)));
    }
  }

  return $env;
}

/**
 * Get the password of your dreams.
 */
function buck_generate_password($length = 10) {
  $chars =  'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz'.
            '0123456789``-=~!@#$%^&*()_+,.<>?;:[]{}\|';

  $str = '';
  $max = strlen($chars) - 1;

  for ($i=0; $i < $length; $i++)
    $str .= $chars[rand(0, $max)];

  return $str;
}
