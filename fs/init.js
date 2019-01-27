load('api_config.js');
load('api_dash.js');
load('api_gpio.js');
load('api_mqtt.js');
load('api_rpc.js');
load('api_shadow.js');
load('api_timer.js');
load('api_sys.js');

let triggerPin = Cfg.get('app.trig_pin');
let echoPin = Cfg.get('app.echo_pin');
let led = Cfg.get('board.led1.pin');              // Built-in LED GPIO number
let state = { uptime: Sys.uptime() };
let getDistance = ffi('double getDistance(int, int, bool)');

GPIO.set_mode(triggerPin, GPIO.MODE_OUTPUT);
GPIO.set_mode(echoPin, GPIO.MODE_INPUT);
GPIO.set_mode(led, GPIO.MODE_OUTPUT);
GPIO.write(led, 1)

let reportState = function () {
  Shadow.update(0, state);
};

// Update state every second, and report to cloud if online
Timer.set(15 * 1000, Timer.REPEAT, function () {
  GPIO.write(led, 0);

  state.distance = getDistance(triggerPin, echoPin, false);
  state.uptime = Sys.uptime();
  state.ram_free = Sys.free_ram();
  print('state:', JSON.stringify(state));
  reportState();

  MQTT.pub('metrics/salt/distance', JSON.stringify(state.distance));

  GPIO.write(led, 1);
}, null);

RPC.addHandler('Distance', function (args) {
  return getDistance(triggerPin, echoPin, true);
});
