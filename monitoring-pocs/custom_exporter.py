import http.server
import psutil

class MetricsHandler(http.server.BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == '/metrics':
            self.send_response(200)
            self.send_header('Content-type', 'text/plain; version=0.0.4')
            self.end_headers()

            cpu = psutil.cpu_percent()
            mem = psutil.virtual_memory().percent
            disk = psutil.disk_usage('/').percent

            metrics = f"""# HELP custom_cpu_usage_percent CPU usage
# TYPE custom_cpu_usage_percent gauge
custom_cpu_usage_percent {cpu}

# HELP custom_memory_usage_percent Memory usage
# TYPE custom_memory_usage_percent gauge
custom_memory_usage_percent {mem}

# HELP custom_disk_usage_percent Disk usage
# TYPE custom_disk_usage_percent gauge
custom_disk_usage_percent {disk}
"""
            self.wfile.write(metrics.encode('utf-8'))
        else:
            self.send_response(404)
            self.end_headers()

if __name__ == '__main__':
    port = 9100
    server = http.server.HTTPServer(('', port), MetricsHandler)
    print(f"Exporter listening on port {port}")
    server.serve_forever()
