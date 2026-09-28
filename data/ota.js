/* --------------------------------------------------------------------------
   1. Upload source mode
   -------------------------------------------------------------------------- */
function toggleMode(mode) {
  const fileGroup = document.getElementById("fileInputGroup");
  const folderGroup = document.getElementById("folderInputGroup");

  fileGroup.hidden = mode !== "file";
  folderGroup.hidden = mode !== "folder";
  document.getElementById("file").value = "";
  document.getElementById("folder").value = "";
  updateSelectedFiles([]);
}

/* --------------------------------------------------------------------------
   2. Payload selection and display
   -------------------------------------------------------------------------- */
function updateSelectedFiles(files) {
  const display = document.getElementById("file-name");
  if (!files || files.length === 0) {
    display.textContent = "No file chosen";
  } else if (files.length === 1) {
    display.textContent = files[0].name;
  } else {
    display.textContent = `${files.length} files selected`;
  }
}

function initializePayloadSelection() {
  const fileInput = document.getElementById("file");
  const folderInput = document.getElementById("folder");

  fileInput.addEventListener("change", () => updateSelectedFiles(fileInput.files));
  folderInput.addEventListener("change", () => updateSelectedFiles(folderInput.files));
  document.getElementById("radioFile").addEventListener("change", () => toggleMode("file"));
  document.getElementById("radioFolder").addEventListener("change", () => toggleMode("folder"));
  toggleMode("file");
}

/* --------------------------------------------------------------------------
   3. Upload progress and request workflow
   -------------------------------------------------------------------------- */
function updateProgress(value) {
  const progress = document.getElementById("progress");
  const progressValue = document.getElementById("progress-value");
  const boundedValue = Math.max(0, Math.min(100, value));

  progress.value = boundedValue;
  progressValue.textContent = `${Math.round(boundedValue)}%`;
}

function setUploading(isUploading) {
  const button = document.getElementById("upload-button");
  button.disabled = isUploading;
  button.textContent = isUploading ? "Uploading..." : "Start flashing";
}

function upload() {
  const fileInput = document.getElementById("file");
  const folderInput = document.getElementById("folder");
  const isFolderMode = document.getElementById("radioFolder").checked;
  const files = Array.from(isFolderMode ? folderInput.files : fileInput.files);
  const type = document.getElementById("type").value;

  if (files.length === 0) {
    alert("Please select files or folder to upload");
    return;
  }

  const totalSize = files.reduce((sum, file) => sum + file.size, 0);
  let uploaded = 0;
  updateProgress(0);
  setUploading(true);

  function uploadFile(index) {
    if (index >= files.length) {
      updateProgress(100);
      setUploading(false);
      alert("All uploads complete!");
      return;
    }

    const file = files[index];
    const path = file.webkitRelativePath || file.name;
    const formData = new FormData();
    formData.append("file", file, path);
    formData.append("type", type);

    const request = new XMLHttpRequest();
    request.upload.onprogress = (event) => {
      if (event.lengthComputable) {
        const currentProgress = totalSize === 0
          ? 100
          : ((uploaded + event.loaded) / totalSize) * 100;
        updateProgress(currentProgress);
      }
    };

    request.onload = () => {
      if (request.status === 200) {
        uploaded += file.size;
        console.log(`[Success] Uploaded "${path}" (${index + 1}/${files.length})`);
        uploadFile(index + 1);
        return;
      }

      console.error(`[Failed] Upload failed for "${path}" with status ${request.status}`);
      setUploading(false);
      updateProgress(0);
      alert("Upload failed: " + request.responseText);
    };

    request.onerror = () => {
      setUploading(false);
      updateProgress(0);
      alert("Network error while uploading " + path);
    };

    request.open("POST", `/${type}`);
    request.send(formData);
  }

  uploadFile(0);
}

/* --------------------------------------------------------------------------
   4. Page initialization
   -------------------------------------------------------------------------- */
function initializeOtaPage() {
  initializePayloadSelection();
  document.getElementById("upload-button").addEventListener("click", upload);
}

document.addEventListener("DOMContentLoaded", initializeOtaPage);