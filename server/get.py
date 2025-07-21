import os
import datetime
import threading
import logging
from logging.handlers import RotatingFileHandler
from functools import wraps
from flask import Flask, request, jsonify, send_from_directory, abort, render_template_string, Response
from werkzeug.utils import secure_filename
from werkzeug.security import check_password_hash, generate_password_hash
from base64 import b64decode
from typing import Optional

app = Flask(__name__)

UPLOAD_ROOT = "received_files"
MAX_FILE_SIZE = 20 * 1024 * 1024  #20 MB per file
MAX_FILES_PER_DAY = 1000
ALLOWED_EXTENSIONS = set([
    'txt', 'log', 'json', 'html', 'xml', 'bin', 'db', 'sqlite', 'sqlite3', 'db-journal',
    'png', 'jpg', 'jpeg', 'pdf', 'doc', 'docx', 'xls', 'xlsx', 'csv', 'dat',
])

DEBUG_MODE = False
HOST = '0.0.0.0'
PORT = 5000

#change username and password for real environment
AUTH_USERNAME = "admin"
#generate with: generate_password_hash("your_password")
AUTH_PASSWORD_HASH = generate_password_hash("changeme123")

#LOGIN
#SET
#UP

if not os.path.exists("logs"):
    os.mkdir("logs")

log_handler = RotatingFileHandler("logs/server.log", maxBytes=10*1024*1024, backupCount=5)
log_handler.setLevel(logging.INFO)
formatter = logging.Formatter('[%(asctime)s] %(levelname)s: %(message)s')
log_handler.setFormatter(formatter)

app.logger.addHandler(log_handler)
app.logger.setLevel(logging.INFO)

#utils
#from here on im too lazy to explain more shit.
#bye

def allowed_file(filename: str) -> bool:
    if '.' not in filename:
        return False
    ext = filename.rsplit('.', 1)[1].lower()
    return ext in ALLOWED_EXTENSIONS

def ensure_dir(path: str):
    if not os.path.exists(path):
        os.makedirs(path)

def check_auth(username: str, password: str) -> bool:
    if username == AUTH_USERNAME and check_password_hash(AUTH_PASSWORD_HASH, password):
        return True
    return False

def authenticate():
    """Sends a 401 response that enables basic auth"""
    return Response(
        'Authentication required', 401,
        {'WWW-Authenticate': 'Basic realm="Login Required"'}
    )

def requires_auth(f):
    @wraps(f)
    def decorated(*args, **kwargs):
        auth = request.authorization
        if not auth or not check_auth(auth.username, auth.password):
            return authenticate()
        return f(*args, **kwargs)
    return decorated

def log_event(message: str, level=logging.INFO):
    app.logger.log(level, message)

def file_count_in_folder(folder: str) -> int:
    if not os.path.exists(folder):
        return 0
    return len([name for name in os.listdir(folder) if os.path.isfile(os.path.join(folder, name))])

def human_readable_size(size: int) -> str:
    for unit in ['B','KB','MB','GB','TB']:
        if size < 1024:
            return f"{size:.2f}{unit}"
        size /= 1024
    return f"{size:.2f}PB"

def list_files_in_folder(folder: str):
    if not os.path.exists(folder):
        return []
    return sorted([f for f in os.listdir(folder) if os.path.isfile(os.path.join(folder, f))])


@app.route('/upload', methods=['POST'])
def upload_file():
    if 'file' not in request.files:
        log_event("Upload failed: No file part in request", logging.WARNING)
        return jsonify({"error": "No file part"}), 400

    file = request.files['file']
    if file.filename == '':
        log_event("Upload failed: No selected file", logging.WARNING)
        return jsonify({"error": "No selected file"}), 400

    filename = secure_filename(file.filename)
    if not allowed_file(filename):
        log_event(f"Upload failed: File type not allowed: {filename}", logging.WARNING)
        return jsonify({"error": "File type not allowed"}), 400

    if request.content_length is not None and request.content_length > MAX_FILE_SIZE:
        log_event(f"Upload failed: File too large: {filename}", logging.WARNING)
        return jsonify({"error": "File too large"}), 413

    date_folder = datetime.datetime.now().strftime("%Y-%m-%d")
    folder_path = os.path.join(UPLOAD_ROOT, date_folder)
    ensure_dir(folder_path)

    if file_count_in_folder(folder_path) >= MAX_FILES_PER_DAY:
        log_event(f"Upload failed: Daily file limit reached ({MAX_FILES_PER_DAY})", logging.WARNING)
        return jsonify({"error": "Daily file limit reached"}), 429

    save_path = os.path.join(folder_path, filename)

    try:
        file.save(save_path)
    except Exception as e:
        log_event(f"Error saving file {filename}: {e}", logging.ERROR)
        return jsonify({"error": "Failed to save file"}), 500

    size = os.path.getsize(save_path)
    log_event(f"Received file: {filename} ({human_readable_size(size)})")
    return jsonify({"success": True, "filename": filename, "size": size}), 201

@app.route('/files/<date>', methods=['GET'])
@requires_auth
def list_files(date):
    folder_path = os.path.join(UPLOAD_ROOT, date)
    if not os.path.exists(folder_path):
        log_event(f"File listing failed: Date folder not found: {date}", logging.WARNING)
        return jsonify({"error": "Date folder not found"}), 404

    files = list_files_in_folder(folder_path)
    log_event(f"Listed {len(files)} files for date {date}")
    return jsonify({"date": date, "files": files})

@app.route('/download/<date>/<filename>', methods=['GET'])
@requires_auth
def download_file(date, filename):
    folder_path = os.path.join(UPLOAD_ROOT, date)
    if not os.path.exists(folder_path):
        log_event(f"Download failed: Date folder not found: {date}", logging.WARNING)
        return jsonify({"error": "Date folder not found"}), 404

    safe_filename = secure_filename(filename)
    file_path = os.path.join(folder_path, safe_filename)

    if not os.path.isfile(file_path):
        log_event(f"Download failed: File not found: {file_path}", logging.WARNING)
        return jsonify({"error": "File not found"}), 404

    log_event(f"File downloaded: {safe_filename}")
    return send_from_directory(folder_path, safe_filename, as_attachment=True)

@app.route('/stats', methods=['GET'])
@requires_auth
def stats():
    stats_data = {}
    total_files = 0

    if not os.path.exists(UPLOAD_ROOT):
        return jsonify({"total_files": 0, "by_date": {}})

    for folder in sorted(os.listdir(UPLOAD_ROOT)):
        folder_path = os.path.join(UPLOAD_ROOT, folder)
        if os.path.isdir(folder_path):
            files = list_files_in_folder(folder_path)
            count = len(files)
            total_files += count
            stats_data[folder] = count

    return jsonify({"total_files": total_files, "by_date": stats_data})

@app.route('/health', methods=['GET'])
def health_check():
    return jsonify({"status": "ok"})

INDEX_HTML = """
<!DOCTYPE html>
<html lang="es">
<head>
<meta charset="UTF-8" />
<meta name="viewport" content="width=device-width, initial-scale=1" />
<title>b0nec0me Server</title>
<style>
body { font-family: Arial, sans-serif; margin: 20px; background:#111; color:#eee; }
h1 { color:#f44; }
a { color: #6af; text-decoration:none; }
a:hover { text-decoration:underline; }
table { border-collapse: collapse; width: 100%; margin-top: 15px; }
th, td { border: 1px solid #444; padding: 8px; text-align: left; }
th { background: #222; }
tr:nth-child(even) { background: #1a1a1a; }
</style>
</head>
<body>
<h1>b0nec0me Server - Files received</h1>
<div>
    <h2>Available dates</h2>
    <ul>
        {% for date in dates %}
            <li><a href="/view/{{ date }}">{{ date }}</a></li>
        {% else %}
            <li>There are no available dates.</li>
        {% endfor %}
    </ul>
</div>
<div>
    <h2>View: </h2>
    <p>Files: {{ total_files }}</p>
</div>
</body>
</html>
"""

VIEW_HTML = """
<!DOCTYPE html>
<html lang="es">
<head>
<meta charset="UTF-8" />
<meta name="viewport" content="width=device-width, initial-scale=1" />
<title>Files of {{ date }}</title>
<style>
body { font-family: Arial, sans-serif; margin: 20px; background:#111; color:#eee; }
h1 { color:#f44; }
a { color: #6af; text-decoration:none; }
a:hover { text-decoration:underline; }
table { border-collapse: collapse; width: 100%; margin-top: 15px; }
th, td { border: 1px solid #444; padding: 8px; text-align: left; }
th { background: #222; }
tr:nth-child(even) { background: #1a1a1a; }
</style>
</head>
<body>
<h1>Files received on {{ date }}</h1>
<div>
    {% if files %}
    <table>
        <thead>
            <tr><th>Files</th><th>Size</th><th>Download</th></tr>
        </thead>
        <tbody>
        {% for file in files %}
            <tr>
                <td>{{ file }}</td>
                <td>{{ sizes[file] }}</td>
                <td><a href="/download/{{ date }}/{{ file }}">Download</a></td>
            </tr>
        {% endfor %}
        </tbody>
    </table>
    {% else %}
        <p>No files found for this date.</p>
    {% endif %}
</div>
<a href="/">Back</a>
</body>
</html>
"""

@app.route('/')
@requires_auth
def index():
    if not os.path.exists(UPLOAD_ROOT):
        dates = []
    else:
        dates = sorted([d for d in os.listdir(UPLOAD_ROOT) if os.path.isdir(os.path.join(UPLOAD_ROOT, d))])

    total_files = sum(len(list_files_in_folder(os.path.join(UPLOAD_ROOT, d))) for d in dates)
    return render_template_string(INDEX_HTML, dates=dates, total_files=total_files)

@app.route('/view/<date>')
@requires_auth
def view_files(date):
    folder_path = os.path.join(UPLOAD_ROOT, date)
    if not os.path.exists(folder_path):
        abort(404)

    files = list_files_in_folder(folder_path)
    sizes = {f: human_readable_size(os.path.getsize(os.path.join(folder_path, f))) for f in files}

    return render_template_string(VIEW_HTML, date=date, files=files, sizes=sizes)

@app.errorhandler(404)
def not_found_error(error):
    return jsonify({"error": "Not Found"}), 404

@app.errorhandler(405)
def method_not_allowed(error):
    return jsonify({"error": "Method Not Allowed"}), 405

@app.errorhandler(413)
def request_entity_too_large(error):
    return jsonify({"error": "File Too Large"}), 413

@app.errorhandler(500)
def internal_error(error):
    return jsonify({"error": "Internal Server Error"}), 500

def run_server():
    log_event("Starting b0nec0me server...")
    app.run(host=HOST, port=PORT, debug=DEBUG_MODE, threaded=True)

if __name__ == "__main__":
    run_server()
