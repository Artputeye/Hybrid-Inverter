// Toggle file/folder input
function toggleMode(mode) {
  const fileGroup = document.getElementById("fileInputGroup");
  const folderGroup = document.getElementById("folderInputGroup");

  if (mode === "file") {
    fileGroup.classList.remove("hidden");
    folderGroup.classList.add("hidden");
  } else {
    fileGroup.classList.add("hidden");
    folderGroup.classList.remove("hidden");
  }

  document.getElementById("file-name").textContent = "No file chosen";
}

// Show selected file/folder info
document.addEventListener("DOMContentLoaded", () => {
  const fileInput = document.getElementById("file");
  const folderInput = document.getElementById("folder");
  const display = document.getElementById("file-name");

  function updateDisplay(files) {
    if (!files || files.length === 0) display.textContent = "No file chosen";
    else if (files.length === 1) display.textContent = files[0].name;
    else display.textContent = files.length + " files selected";
  }

  fileInput.addEventListener("change", () => updateDisplay(fileInput.files));
  folderInput.addEventListener("change", () => updateDisplay(folderInput.files));
});

// Upload function
function upload() {
  const fileInput = document.getElementById("file");
  const folderInput = document.getElementById("folder");
  const type = document.getElementById("type").value;
  const progress = document.getElementById("progress");

  let files = [];
  if (document.getElementById("radioFolder").checked && folderInput.files.length > 0) {
    files = Array.from(folderInput.files);
  } else if (document.getElementById("radioFile").checked && fileInput.files.length > 0) {
    files = Array.from(fileInput.files);
  } else {
    alert("Please select files or folder to upload");
    return;
  }

  let totalSize = files.reduce((sum, f) => sum + f.size, 0);
  let uploaded = 0;

  function uploadFile(index) {
    if (index >= files.length) {
      alert("All uploads complete!");
      progress.value = 0;
      return;
    }

    const file = files[index];
    const path = file.webkitRelativePath || file.name;

    const formData = new FormData();
    formData.append("file", file, path);
    formData.append("type", type);

    const xhr = new XMLHttpRequest();
    xhr.upload.onprogress = e => {
      if (e.lengthComputable) {
        progress.value = ((uploaded + e.loaded) / totalSize) * 100;
      }
    };

    xhr.onload = () => {
      if (xhr.status === 200) {
        uploaded += file.size;
        uploadFile(index + 1);
      } else {
        alert("Upload failed: " + xhr.responseText);
        progress.value = 0;
      }
    };

    xhr.onerror = () => {
      alert("Network error while uploading " + path);
      progress.value = 0;
    };

    xhr.open("POST", "/" + type);
    xhr.send(formData);
  }

  uploadFile(0);
}
