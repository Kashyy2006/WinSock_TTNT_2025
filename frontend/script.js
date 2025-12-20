let SERVER_IP = "";
const PORT = 6969;

function switchTab(tabId, btn) {
  document
    .querySelectorAll(".page")
    .forEach((p) => p.classList.remove("active"));
  document
    .querySelectorAll(".nav-btn")
    .forEach((b) => b.classList.remove("active"));
  document.getElementById(tabId).classList.add("active");
  btn.classList.add("active");
}

////////////////////////
// 1. Hàm bật giao diện Camera (startOnWebcam)
let streamInterval = null;

async function startOnWebcam() {
  await fetch(`http://${SERVER_IP}:${PORT}/webcam/start`);
  const container = document.getElementById("onWebcamContainer");
  container.style.display = "flex";

  if (streamInterval) clearInterval(streamInterval);
  // Giữ nguyên frame cũ cho tới khi frame mới sẵn sàng
  streamInterval = setInterval(() => {
    const img = document.getElementById("camStream");
    const ts = new Date().getTime();
    if (!img) {
      container.innerHTML = `<img id="camStream" src="http://${SERVER_IP}:${PORT}/snapshot_stream?t=${ts}" 
                                     style="width:100%; height:100%; object-fit:contain; border-radius:8px;">`;
    } else {
      img.src = `http://${SERVER_IP}:${PORT}/snapshot_stream?t=${ts}`;
    }
  }, 300); // ~10FPS
  showToast("Camera mode on!", "success");
}

async function stopOnWebcam() {
  await fetch(`http://${SERVER_IP}:${PORT}/webcam/stop`);
  if (streamInterval) clearInterval(streamInterval);
  streamInterval = null;

  const container = document.getElementById("onWebcamContainer");
  container.innerHTML =
    '<span style="color: #64748b;">Màn hình Camera (Đã tắt)</span>';
  showToast("Camera mode off!", "error");
}

// Sửa hàm postControl để ghép tham số vào URL (Vì C++ của bạn đọc query)
async function postControl(pathStr, params = {}) {
  try {
    // Chuyển object params thành chuỗi query: ?seconds=5
    const queryStr = new URLSearchParams(params).toString();
    const url = `http://${SERVER_IP}:${PORT}${pathStr}?${queryStr}`;

    const response = await fetch(url, {
      method: "POST", // Vẫn giữ POST nhưng gửi kèm Query Param
      headers: { "Content-Type": "text/plain" },
    });
    return await response.text();
  } catch (error) {
    console.error("Lỗi kết nối:", error);
    return "Error: Không kết nối được Server";
  }
}

// 3. Hàm gửi lệnh quay (sendCommand) - Đã sửa lỗi "sec is not defined"
async function sendCommand(cmd) {
  if (cmd === "recordVideo") {
    const secInput = document.getElementById("recSeconds");
    const sec = secInput ? parseInt(secInput.value) : 5;

    stopOnWebcam();

    showToast("Đang chuẩn bị Camera...", "info");
    const resultDiv = document.getElementById("recordResult");
    if (resultDiv)
      resultDiv.innerHTML = "<div>⏳ Đang giải phóng Camera...</div>";

    setTimeout(async () => {
      showToast(`🎥 Bắt đầu quay ${sec}s...`, "info");
      if (resultDiv)
        resultDiv.innerHTML = `<div style="color:cyan">🔴 Đang quay video (${sec}s)...</div>`;

      const path = await postControl("/webcam", { seconds: sec });

      // Xử lý kết quả như cũ...
      if (path.includes("Error")) {
        if (resultDiv) resultDiv.innerHTML = "Lỗi Server/Camera bận";
        return;
      }

      // Chờ file tạo xong
      setTimeout(() => {
        const fullUrl = `http://${SERVER_IP}:${PORT}${path}`;
        if (resultDiv) {
          resultDiv.innerHTML = `
                    <div style="background:#1e293b; padding:10px; border-radius:8px; margin-top:5px;">
                        <p style="color:#4ade80">✅ Xong!</p>
                        <a href="${fullUrl}" class="action-btn btn-primary">Tải Video</a>
                    </div>`;
        }
      }, sec * 1000 + 1000);
    }, 1000); // Delay 1s
  }
}
//////////////

function triggerScreenshot() {
  document.getElementById("mediaResult").innerText = "Taking screenshot...";
  fetch("/screenshot")
    .then((res) => res.text())
    .then((data) => {
      document.getElementById("mediaResult").innerText = data;
    });
}

// Hàm hiển thị thông báo (Toast)
function showToast(msg, type = "success") {
  const container = document.getElementById("toast-container");
  const toast = document.createElement("div");
  toast.className = `toast ${type}`;
  toast.innerHTML = `<i class="fa-solid ${
    type === "error" ? "fa-circle-exclamation" : "fa-check"
  }"></i> ${msg}`;
  container.appendChild(toast);

  setTimeout(() => {
    toast.style.animation = "slideIn 0.3s reverse";
    setTimeout(() => toast.remove(), 300);
  }, 3000);
}

// Hàm chuyển Tab
function switchTab(tabId, btn) {
  document
    .querySelectorAll(".page")
    .forEach((p) => p.classList.remove("active"));
  document
    .querySelectorAll(".nav-btn")
    .forEach((b) => b.classList.remove("active"));
  document.getElementById(tabId).classList.add("active");
  btn.classList.add("active");
}

// --- 2. LOGIC MẠNG (Kết nối C++) ---

async function sendRequest(route) {
  // Kiểm tra kết nối
  if (!SERVER_IP) {
    showToast("You are not connected to server!", "error");
    return null;
  }
  // Kiểm tra route có hợp lệ không (Sửa lỗi 404)
  if (!route) {
    console.error("Error: Route is empty (Undefined command)");
    return null;
  }

  try {
    const url = `http://${SERVER_IP}:${PORT}${route}`;
    const controller = new AbortController();
    const timeoutId = setTimeout(() => controller.abort(), 10000); // Tăng timeout lên 10s cho video

    const response = await fetch(url, {
      method: "GET",
      signal: controller.signal,
    });

    clearTimeout(timeoutId);

    if (!response.ok) {
      throw new Error(`Server Error: ${response.status}`);
    }
    return await response.text();
  } catch (e) {
    console.error(e);
    showToast(`Connection Error: ${e.message}`, "error");
    return null;
  }
}

// --- 3. XỬ LÝ KẾT NỐI (Connect/Disconnect) ---
document.getElementById("btnConnect").addEventListener("click", async () => {
  const ipInput = document.getElementById("ipInput").value.trim();
  if (!ipInput) return showToast("Please enter IP!", "error");

  SERVER_IP = ipInput;
  const res = await sendRequest("/ping");

  if (res) {
    showToast("Connected!");
    document.getElementById(
      "connectStatus"
    ).innerHTML = `Connected: <span style="color:var(--success)">${SERVER_IP}</span>`;
    document.getElementById("connectionPanel").style.display = "none";
    document.getElementById("disconnectPanel").style.display = "flex";
  } else {
    SERVER_IP = "";
  }
});

document.getElementById("btnDisconnect").addEventListener("click", () => {
  SERVER_IP = "";
  document.getElementById("connectStatus").innerHTML = "Status: Disconnected";
  document.getElementById("connectionPanel").style.display = "flex";
  document.getElementById("disconnectPanel").style.display = "none";
  showToast("Disconnected!");
});

// --- 4. GỬI LỆNH (Main Logic) ---

async function sendCommand(cmd) {
  let route = "";

  // --- Xử lý các lệnh cơ bản ---
  if (cmd === "listApp") route = "/apps";
  else if (cmd === "listProcess") route = "/processes";
  else if (cmd === "shutdown") route = "/shutdown";
  else if (cmd === "restart") route = "/restart";
  else if (cmd === "getKeylog") route = "/keylogger/get";
  else if (cmd === "startApp") {
    const name = document.getElementById("appName").value;
    if (!name) return showToast("Enter name!", "error");
    route = `/apps/start?name=${encodeURIComponent(name)}`;
  } else if (cmd === "stopApp") {
    const name = document.getElementById("appName").value;
    if (!name) return showToast("Enter name!", "error");
    route = `/apps/stop?name=${encodeURIComponent(name)}`;
  } else if (cmd === "stopProcess") {
    const pid = document.getElementById("processName").value;
    if (!pid) return showToast("Enter PID or name!", "error");
    route = `/processes/stop?name=${encodeURIComponent(pid)}`;
  }

  // --- XỬ LÝ SCREENSHOT (Sửa để hiển thị ảnh) ---
  else if (cmd === "screenshot") {
    showToast("📸 Screenshoting...", "info");
    const path = await sendRequest("/screenshot"); // Server trả về "/screenshot.bmp"

    if (path) {
      const timestamp = new Date().getTime();
      const imgUrl = `http://${SERVER_IP}:${PORT}${path.trim()}?t=${timestamp}`;

      document.getElementById("mediaResult").innerHTML = `
                <div style="margin-top:10px; text-align: center;">
                    <img src="${imgUrl}" style="max-width: 100%; border-radius: 8px; border: 1px solid #475569; box-shadow: 0 4px 6px rgba(0,0,0,0.3);" />
                    <br>
                    <a href="${imgUrl}" download="screenshot_${timestamp}.bmp" class="action-btn" style="margin-top: 5px; display: inline-block; background: #334155;">
                        <i class="fa-solid fa-download"></i> Tải ảnh về
                    </a>
                </div>
            `;
      showToast("Done!");
    }
    return;
    return; // Kết thúc hàm, không chạy phần default ở dưới
  }

  // --- XỬ LÝ WEBCAM (Sửa lỗi 404) ---
  // HTML gọi là 'recordVideo', nên ta bắt case này
  else if (cmd === "recordVideo") {
    const secInput = document.getElementById("recSeconds");
    const sec = secInput ? secInput.value : 5;

    showToast(`🎥 Recording ${sec}s...`, "info");
    document.getElementById(
      "recordResult"
    ).innerHTML = `<span style="color:var(--warning)">⏳ Đang quay video... vui lòng chờ ${sec}s</span>`;

    // Gọi lệnh xuống C++ (Dạng GET cho đơn giản)
    const path = await sendRequest(`/webcam?seconds=${sec}`);

    if (path && !path.includes("Error")) {
      // Chờ thêm 1 chút để server chắc chắn ghi xong file
      setTimeout(() => {
        const timestamp = new Date().getTime();
        const fullUrl = `http://${SERVER_IP}:${PORT}${path.trim()}?t=${timestamp}`;
        const downloadName = `Evidence_${timestamp}.avi`;

        document.getElementById("recordResult").innerHTML = `
                    <div style="background:#1e293b; padding:15px; border-radius:8px; margin-top:10px; border: 1px solid #475569;">
                        <p style="color: #4ade80; margin-bottom: 10px;">✅ Quay thành công!</p>
                        <a href="${fullUrl}" download="${downloadName}" class="action-btn btn-primary" style="text-decoration: none; display: inline-block;">
                            <i class="fa-solid fa-download"></i> Tải Video (.avi)
                        </a>
                    </div>`;
        showToast("Video is ready!", "success");
      }, sec * 1000 + 500);
    } else {
      document.getElementById(
        "recordResult"
      ).innerHTML = `<span style="color:var(--danger)">Lỗi khi quay video.</span>`;
    }
    return;
  }

  // --- Gửi request cho các lệnh thường (listApp, listProcess...) ---
  if (route) {
    const result = await sendRequest(route);
    if (result) {
      if (cmd === "listApp") {
        document.getElementById("appList").innerHTML = result;
        showToast("Apps list updated!");
      } else if (cmd === "listProcess") {
        document.getElementById("processList").innerHTML = result;
        showToast("Processes list updated!");
      } else if (cmd === "getKeylog") {
        document.getElementById("keylogResult").innerText =
          result || "Chưa có dữ liệu...";
      } else {
        // Các lệnh start/stop chỉ cần hiện thông báo
        showToast(result);
      }
    }
  }
}
