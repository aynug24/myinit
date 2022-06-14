INIT_DIR="/tmp/myinit/"
CONFIG_PATH="${INIT_DIR}config"
LOG_PATH="${INIT_DIR}myinit.log"
RESULT_PATH="${INIT_DIR}result.txt"
DEVNULL="/dev/null"


echo "Building myinit..."
# make clear --silent
make all --silent || { echo "Build failure"; exit 1; }
echo "Successfully built myinit!"

killall -q -SIGTERM myinit

# myinit log directory is /tmp/myinit/, it can create it itself though
# clear dir before testing
test -d $INIT_DIR && find $INIT_DIR -delete
mkdir -p $INIT_DIR

exec > >(tee -i $RESULT_PATH)  # tee all output to /tmp/myinit/result.txt

function test_start() {
  echo
  echo "TEST 1"
  echo "Myinit should start tasks specified in config file."
  echo "For this test myinit will start with sleep, ping, and find tasks"
  echo "After sleeping for some time, we'll check that myinit has three child processes"
  echo

  three_task_config="$(which sleep) 15 $DEVNULL $DEVNULL
  $(which ping) 127.0.0.1 -c 10 $DEVNULL /tmp/myinit/first_ping.txt
  $(which find) / -name .gitignore $DEVNULL /tmp/myinit/find.txt"

  echo -n "$three_task_config" >$CONFIG_PATH

  myinit_pid=$(./myinit -c "$CONFIG_PATH") # myinit outputs daemon pid
  [ -z "$myinit_pid" ] && { echo "Failed to start myinit"; exit 1; }

  sleep 1

  running_children=$(ps --ppid "$myinit_pid" --no-headers)
  running_children_count=$(echo "$running_children" | wc -l)

  echo "Expected: 3 children"
  echo "Got: ${running_children_count} children"
  if [[ $running_children_count -eq 3 ]]; then
    echo "SUCCESS!"
  else
    echo "FAIL!"
  fi
}

function test_restart() {
  echo
  echo "TEST 2"
  echo "Myinit should track tasks and should restart them on exit or termination."
  echo "For this test we will terminate the ping task, wait for some time,"
  echo "and check whether myinit restarted ping."
  echo

  # couldn't use pkill, killing the last sleep process (that's what it's for?) is error-prone
  ping_pid=$(echo "$running_children" | grep ping | awk '{print $1}')
  kill "$ping_pid"

  sleep 1

  running_children_count=$(ps --ppid "$myinit_pid" --no-headers | wc -l)

  echo "Expected: 3 children"
  echo "Got: ${running_children_count} children"
  if [[ $running_children_count -eq 3 ]]; then
    echo "SUCCESS!"
  else
    echo "FAIL!"
  fi
}

function test_reset() {
  echo
  echo "TEST 3"
  echo "On receiving SIGHUP myinit should terminate all tasks, reread config file,"
  echo "and start tasks from the new config."
  echo "For this test we will write single ping task to config and SIGHUP myinit process."
  echo "After some time myinit should start one task."
  echo

  one_task_config="$(which ping) 127.0.0.1 -c 7 $DEVNULL /tmp/myinit/second_ping.txt"

  echo -n "$one_task_config" >$CONFIG_PATH

  kill -SIGHUP "$myinit_pid"

  sleep 1

  running_children_count=$(ps --ppid "$myinit_pid" --no-headers | wc -l)

  echo "Expected: 1 child(-ren)"
  echo "Got: ${running_children_count} child(-ren)"

  if [[ $running_children_count -eq 1 ]]; then
    echo "SICCESS!"
  else
    echo "FAIL!"
  fi
}

function test_logs() {
  echo
  echo "TEST 4"
  echo "Myinit should log task starts and exits/teminations."
  echo "For this test we will teminate myinit, wait for log flush,"
  echo "and check whether log entries conform to actions from tests 1-3."
  echo

  kill -SIGTERM "$myinit_pid"

  sleep 0.5

  # Start x3, Terminated (with pid from idx=1), Start (restart of terminated)
  # Terminated x3 (from SIGHUP), Start (from new config)
  log_pattern="^(?:.*Started task.*)
(?:.*Started task idx=1.* with pid=(?'ping_pid'\d+))
(?:.*Started task.*)
(?:.*Child process with pid=\g{ping_pid} was terminated.*)
(?:.*Started task idx=1.*)
(?:.*Child process .* was terminated.*)
(?:.*Child process .* was terminated.*)
(?:.*Child process .* was terminated.*)
(?:.*Started task idx=0.*)\n?$"

  log_pattern_esaped_newline=$(echo "$log_pattern" | sed '$!s/$/\\n/' | tr -d '\n')

  log_start_and_term_entries=$(grep -i -e 'started task' -e 'was terminated' $LOG_PATH)

  echo "Expected: $(wc -l <<< "$log_pattern") lines"
  echo "Got: $(wc -l <<< "$log_start_and_term_entries") lines"

  if grep -q -Pz "$log_pattern_esaped_newline" <<< "$log_start_and_term_entries"; then
    echo "Lines matched pattern."
    echo "SUCCESS!"
  else
    echo "Lines did not match pattern."
    echo "FAIL!"
  fi
}

test_start
test_restart
test_reset
test_logs
