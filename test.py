import socket
import subprocess
import os
import urllib.parse
import psutil

APPS = {
    "notepad": "notepad.exe",
    "mspaint": "mspaint.exe",
    "cmd": "cmd.exe"
}

def html_page(body):
    return (
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        f"Content-Length: {len(body)}\r\n"
        "Connection: close\r\n\r\n"
        + body
    )

def list_apps():
    rows = ""
    for name, exe in APPS.items():
        # check running
        running = any(proc.name().lower() == exe.lower() for proc in psutil.process_iter())
        if running:
            action_link = f"<li><a href='/stop?app={name}'>Stop</a></li>"
        else:
            action_link = f"<li><a href='/start?app={name}'>Start</a></li>"
        rows += f"<li>{name} - {'running' if running else 'stopped'} | {action_link}</li>"

    return html_page(f"<h1>App List</h1><ul>{rows}</ul>")

def start_app(app_name):
    if app_name not in APPS:
        return html_page(f"<h1>App '{app_name}' not found</h1>")
    if app_name == "chrome":
        subprocess.Popen(["cmd", "/c", "start", "chrome"])
    subprocess.Popen(APPS[app_name])


def stop_app(app_name):
    if app_name not in APPS:
        return html_page(f"<h1>App '{app_name}' not found</h1>")

    exe = APPS[app_name].lower()
    count = 0
    for proc in psutil.process_iter():
        if proc.name().lower() == exe:
            proc.terminate()
            count += 1


def redirect(url="/"):
    return (
        "HTTP/1.1 302 Found\r\n"
        f"Location: {url}\r\n"
        "Content-Length: 0\r\n"
        "\r\n"
    )

# ====================== WEBSERVER ======================

server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server.bind(("0.0.0.0", 8080))
server.listen(5)
print("Server running at http://localhost:8080")

while True:
    client, addr = server.accept()
    request = client.recv(4096).decode()
    print(request)

    # Parse path
    try:
        first_line = request.split("\n")[0]
        _, path, _ = first_line.split()
    except:
        client.close()
        continue

    # Query parse
    parsed = urllib.parse.urlparse(path)
    route = parsed.path
    query = urllib.parse.parse_qs(parsed.query)

    if route == "/":
        response = html_page("<h1>WinSock Python App Controller</h1>"
                             "<ul>"
                             "<li><a href='/list'>List Apps</a></li>"
                             "</ul>")

    elif route == "/list":
        response = list_apps()

    elif route == "/start":
        app = query.get("app", [""])[0]
        response = start_app(app)
        response = redirect("/list")

    elif route == "/stop":
        app = query.get("app", [""])[0]
        response = stop_app(app)
        response = redirect("/list")

    else:
        response = html_page("<h1>404 Not Found</h1>")

    client.sendall(response.encode())
    client.close()
