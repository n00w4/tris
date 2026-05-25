#!/bin/bash
# demo_tmux.sh – Starts 4 client instances in a tmux session for demonstration purposes

SESSION_NAME="tris_demo"
BASE_DIR="demo_clients_tmux"

# clean up any existing tmux session
tmux kill-session -t "$SESSION_NAME" 2>/dev/null

# create base directory for client instances
rm -rf "$BASE_DIR"
mkdir -p "$BASE_DIR"

# list of usernames
USERNAMES=("Alice" "Bob" "Charlie" "Diana")
SERVER_IP="127.0.0.1"
SERVER_PORT="8080"
BIN_PATH="$(realpath ./tris_client)"   # client binary absolute path

# create a directory and config for each client instance
for name in "${USERNAMES[@]}"; do
    CLIENT_DIR="$BASE_DIR/$name"
    mkdir -p "$CLIENT_DIR"
    cat > "$CLIENT_DIR/config.conf" <<EOF
username=$name
ip=$SERVER_IP
port=$SERVER_PORT
EOF
    # create a symlink to the client binary in each client's directory
    ln -sf "$BIN_PATH" "$CLIENT_DIR/tris_client"
done

# start a new tmux session
tmux new-session -d -s "$SESSION_NAME" -n "tris"

# create a tiled layout with 4 panes (2x2)
tmux select-layout -t "$SESSION_NAME" tiled   # 1 pane initially
tmux split-window -h -t "$SESSION_NAME"       # vertical split to create 2 panes
tmux split-window -v -t "$SESSION_NAME:0.0"   # horizontal split in the left pane
tmux split-window -v -t "$SESSION_NAME:0.1"   # horizontal split in the right pane
tmux select-layout -t "$SESSION_NAME" tiled   # force tiled layout

# start each client in its respective pane
INDEX=0
for name in "${USERNAMES[@]}"; do
    CLIENT_DIR="$BASE_DIR/$name"
    tmux send-keys -t "$SESSION_NAME:0.$INDEX" "cd '$CLIENT_DIR' && ./tris_client" C-m
    ((INDEX++))
done

# attach to the tmux session
tmux attach-session -t "$SESSION_NAME"

# cleanup after the session ends
rm -rf "$BASE_DIR"