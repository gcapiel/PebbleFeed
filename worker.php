<?php
$worker= new GearmanWorker();
$worker->addServer();
$worker->addFunction("build_pbw", "build_pbw");
while ($worker->work());
 
function build_pbw($route)
{
  $output = shell_exec('/root/getbus.rb ' . $route->workload() . ' 2>&1');
  $output = shell_exec('/root/BusTimer/waf build 2>&1');
  $output = shell_exec('cp /root/BusTimer/build/BusTimer.pbw /var/www/BusTimer.pbw 2>&1');
  return $output;
}
?>
