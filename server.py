import subprocess
import threading
import time
import re
from flask import Flask, render_template
from flask_socketio import SocketIO

app = Flask(__name__)
app.config['SECRET_KEY'] = 'secret!'
socketio = SocketIO(app, async_mode='eventlet')

# --- CONFIGURATION ---
# CHANGE THIS to the name of your compiled C executable
EXECUTABLE_NAME = "./os_project" 
# ---------------------

@app.route('/')
def index():
    return render_template('index.html')

def run_c_process():
    try:
        # Run the C program
        process = subprocess.Popen(
            [EXECUTABLE_NAME], 
            stdout=subprocess.PIPE, 
            stderr=subprocess.PIPE,
            text=True,
            bufsize=1
        )

        print(f"--- Started {EXECUTABLE_NAME} ---")

        # Read output line by line
        for line in process.stdout:
            line = line.strip()
            print(f"C Output: {line}") 
            
            # Send raw log to UI console
            socketio.emit('console_log', {'data': line})

            # Check for specific patterns to update the visual cards
            # Looking for patterns like "Thread 1 yields" or "Thread 2 created"
            # You can customize this parsing logic based on what your friend's code prints.
            
            if "created" in line.lower():
                # Example: "Thread 1 created"
                parts = re.findall(r'\d+', line)
                if parts:
                    socketio.emit('thread_update', {'id': parts[0], 'state': 'READY', 'details': 'Created'})

            elif "yield" in line.lower() or "switch" in line.lower():
                # Example: "Thread 1 yields"
                parts = re.findall(r'\d+', line)
                if parts:
                    socketio.emit('thread_update', {'id': parts[0], 'state': 'READY', 'details': 'Yielded'})

            elif "running" in line.lower() or "context to" in line.lower():
                # Example: "Switching context to Thread 2"
                parts = re.findall(r'\d+', line)
                if parts:
                    socketio.emit('thread_update', {'id': parts[0], 'state': 'RUNNING', 'details': 'Running'})
            
            elif "exit" in line.lower() or "finished" in line.lower():
                parts = re.findall(r'\d+', line)
                if parts:
                    socketio.emit('thread_update', {'id': parts[0], 'state': 'FINISHED', 'details': 'Exited'})

            time.sleep(0.1) # Small delay to make animation visible

        socketio.emit('simulation_end', {'msg': 'Simulation Complete'})
        
    except FileNotFoundError:
        socketio.emit('console_log', {'data': f"Error: Could not find executable '{EXECUTABLE_NAME}'. Did you compile?"})

@socketio.on('start_simulation')
def handle_start():
    t = threading.Thread(target=run_c_process)
    t.daemon = True
    t.start()

if __name__ == '__main__':
    print("Server running on http://127.0.0.1:5000")
    socketio.run(app, host='0.0.0.0', port=5000)
