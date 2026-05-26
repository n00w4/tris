#!/bin/bash
# stress_test.sh – stressing the server with multiple clients creating/joining games and making moves in a loop

SESSION="stress"
BASE_DIR="clients_stress"
SERVER_IP="127.0.0.1"
SERVER_PORT="8080"
BIN="$(realpath ./tris_client)"
USERNAMES=("A" "B" "C" "D")
ITERATIONS=5   # numbers of iterations of the stress test

# cleanup any existing tmux session and client directories
tmux kill-session -t "$SESSION" 2>/dev/null
rm -rf "$BASE_DIR"
mkdir -p "$BASE_DIR"

# prepare client directories and configs
for i in "${!USERNAMES[@]}"; do
    name="${USERNAMES[$i]}"
    dir="$BASE_DIR/$name"
    mkdir -p "$dir"
    cat > "$dir/config.conf" <<EOF
username=$name
ip=$SERVER_IP
port=$SERVER_PORT
EOF
    ln -sf "$BIN" "$dir/tris_client"
done

# start tmux with 2x2 layout
tmux new-session -d -s "$SESSION" -n "stress"
tmux split-window -h -t "$SESSION"
tmux split-window -v -t "$SESSION:0.0"
tmux split-window -v -t "$SESSION:0.1"
tmux select-layout -t "$SESSION" tiled

# start clients in their panes
for i in "${!USERNAMES[@]}"; do
    name="${USERNAMES[$i]}"
    dir="$BASE_DIR/$name"
    tmux send-keys -t "$SESSION:0.$i" "cd '$dir' && ./tris_client" C-m
    sleep 0.5
done

# give clients time to start and connect to the server
sleep 2

# function to send keys to a specific pane
send_keys() {
    local pane=$1
    local keys=$2
    tmux send-keys -t "$SESSION:0.$pane" "$keys"
}

# stress test loop
for ((iter=1; iter<=ITERATIONS; iter++)); do
    echo "Iteration $iter"
    # phase 1: A and B create games
    send_keys 0 "1"   # A create game
    send_keys 1 "1"   # B create game
    sleep 1

    # phase 2: C and D try to join the games
    send_keys 2 "2"
    sleep 0.5
    send_keys 2 "1"
    send_keys 3 "2"
    sleep 0.5
    send_keys 3 "2"

    sleep 1.5

    # phase 3: The owners accept
    send_keys 0 "y"
    send_keys 1 "y"

    # phase 4: Random moves for a few seconds
    for move in {1..5}; do
        # move on client 2 (player O on A's match) and client 3 (O in B's match)
        # assume cursor is already on cell (0,0). We send Enter.
        send_keys 2 " "
        send_keys 3 " "
        sleep 0.3
        # Alternate turns – not needed, the server handles it. We can move on other cells.
        send_keys 0 " "  # A muove
        send_keys 1 " "  # B muove
        sleep 0.3
    done

    # phase 5: Exit to main menu
    for i in 0 1 2 3; do
        send_keys $i ""
        sleep 0.2
    done
    sleep 1.5

    sleep 1
done

echo "Stress test completed. Press enter to terminate the clients."
read
tmux kill-session -t "$SESSION"
rm -rf "$BASE_DIR"
