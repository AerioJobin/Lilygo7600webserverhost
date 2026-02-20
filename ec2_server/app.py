from flask import Flask, request, render_template_string, send_from_directory
import os, time

app = Flask(__name__)
UPLOAD_FOLDER = 'uploads'
os.makedirs(UPLOAD_FOLDER, exist_ok=True)

# The same Clean UI logic, now in Python
HTML_TEMPLATE = '''
<html><head><meta name="viewport" content="width=device-width, initial-scale=1.0">
<style>
    body{background:#0f172a; color:#f8fafc; font-family:sans-serif; text-align:center; padding:20px;}
    .gallery{display:grid; grid-template-columns: repeat(auto-fill, minmax(150px, 1fr)); gap:15px;}
    .card{background:#1e293b; border-radius:12px; padding:10px; border:1px solid #334155;}
    img{width:100%; height:130px; object-fit:cover; border-radius:8px;}
</style></head><body>
    <h2>Aerio Cloud Gallery</h2>
    <div class="gallery">
        {% for img in images %}
        <div class="card"><img src="/uploads/{{ img }}"><span>{{ img }}</span></div>
        {% endfor %}
    </div>
</body></html>
'''

@app.route('/')
def index():
    images = sorted(os.listdir(UPLOAD_FOLDER), reverse=True)
    return render_template_string(HTML_TEMPLATE, images=images)

@app.route('/upload', methods=['POST'])
def upload():
    file_bytes = request.get_data()
    filename = f"IMG_{int(time.time())}.jpg"
    with open(os.path.join(UPLOAD_FOLDER, filename), 'wb') as f:
        f.write(file_bytes)
    return "OK", 200

@app.route('/uploads/<filename>')
def serve_image(filename):
    return send_from_directory(UPLOAD_FOLDER, filename)

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=80)
