
echo "START"

make clear --silent
make all --silent


echo "MADE"

killall -q -SIGTERM myinit

# myinit log directory is /tmp/myinit/, it can create it itself
# for testing convenience we'll place config there too and rm directory after tests

INIT_DIR="/tmp/myinit/"
CONFIG_PATH="${INIT_DIR}config"

DEVNULL="/dev/null"

find $INIT_DIR -delete
mkdir -p $INIT_DIR

exec > >(tee -ia result.txt)  # tee all output to result.txt

three_task_config="$(which sleep) 15 $DEVNULL $DEVNULL
$(which ping) 127.0.0.1 -c 10 $DEVNULL /tmp/myinit/first_ping.txt
$(which find) / -name .gitignore $DEVNULL /tmp/myinit/find.txt"

echo -n "$three_task_config" >$CONFIG_PATH

myinit_pid=$(./myinit -c "$CONFIG_PATH")

echo "myinit_pid $myinit_pid"

sleep 1

running_children=$(ps --ppid "$myinit_pid" --no-headers)
running_children_count=$(echo "$running_children" | wc -l)
ping_pid=$(echo "$running_children" | grep ping | awk '{print $1}')
echo "running_children $running_children"
echo "running_children_count $running_children_count"
echo "ping_pid $ping_pid"

if [[ $running_children_count -eq 3 ]]; then
  echo "Success!"
else
  echo "Fail!"
fi

# task says to use pkill but killing the last sleep process is dangerous
kill "$ping_pid"

sleep 1

running_children_count=$(ps --ppid "$myinit_pid" --no-headers | wc -l)

if [[ $running_children_count -eq 3 ]]; then
  echo "Success!"
else
  echo "Fail!"
fi

one_task_config="$(which ping) 127.0.0.1 -c 7 $DEVNULL /tmp/myinit/second_ping.txt"

echo -n "$one_task_config" >$CONFIG_PATH

kill -SIGHUP "$myinit_pid"

sleep 1

running_children_count=$(ps --ppid "$myinit_pid" --no-headers | wc -l)
# ps -x --ppid "$myinit_pid" --no-headers
echo "Running children ${running_children_count}"

if [[ $running_children_count -eq 1 ]]; then
  echo "Success!"
else
  echo "Fail!"
fi

kill -SIGTERM "$myinit_pid"