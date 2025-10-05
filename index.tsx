import React, {useState, useEffect} from "react"
import {createRoot} from "react-dom/client"
import * as JUCE from "juce-framework-frontend-mirror"
import {ProgressBar} from "react-bootstrap"
import logo from "./assets/logo.png"
import "bootstrap/dist/css/bootstrap.min.css"
import "./index.scss"

const getState = JUCE.getNativeFunction("getState")
const selectAudio = JUCE.getNativeFunction("selectAudio")
const selectDest = JUCE.getNativeFunction("selectDest")
const updateSkipVocalExtraction = JUCE.getNativeFunction("updateSkipVocalExtraction")
const updateKeepVocalFile = JUCE.getNativeFunction("updateKeepVocalFile")
const startProcessing = JUCE.getNativeFunction("startProcessing")
const openFolder = JUCE.getNativeFunction("openFolder")
const showFileInFolder = JUCE.getNativeFunction("showFileInFolder")

const App: React.FunctionComponent = () => {
    const [audioPath, setAudioPath] = useState("")
    const [destPath, setDestPath] = useState("")
    const [skipVocalExtraction, setSkipVocalExtraction] = useState(false)
    const [keepVocalFile, setKeepVocalFile] = useState(false)
    const [state, setState] = useState("")
    const [progress, setProgress] = useState(100)

    useEffect(() => {
        window.__JUCE__.backend.addEventListener("state-changed", (state: string) => {
            setState(state === "finished" ? "" : state)
        })
        window.__JUCE__.backend.addEventListener("progress", (percent: number) => {
            setProgress((prev) => {
                if (prev === 100) return percent
                return percent >= prev ? percent : prev
            })
        })
        window.__JUCE__.backend.addEventListener("dropped-file", (file: string) => {
            setAudioPath(file)
        })
        window.addEventListener("dragenter", cancelEvent)
        window.addEventListener("dragover", cancelEvent)
        window.addEventListener("dragleave", cancelEvent)
        window.addEventListener("drop", dropEvent)
        initState()
    }, [])

    const initState = async () => {
        const state = await getState()
        setAudioPath(state.audioPath)
        setDestPath(state.destPath)
        setSkipVocalExtraction(state.skipVocalExtraction)
        setKeepVocalFile(state.keepVocalFile)
        setState(state.state === "finished" ? "" : state)
        setProgress((prev) => {
            if (prev === 100) return state.progress
            return state.progress >= prev ? state.progress : prev
        })
    }

    const onSelectAudio = async () => {
        const audio = await selectAudio()
        if (audio) setAudioPath(audio)
    }

    const onSelectDest = async () => {
        const dest = await selectDest()
        if (dest) setDestPath(dest)
    }

    const toggleSkipVocalExtraction = async () => {
        const newValue = !skipVocalExtraction
        updateSkipVocalExtraction(newValue)
        setSkipVocalExtraction(newValue)
    }

    const toggleKeepVocalFile = async () => {
        const newValue = !keepVocalFile
        updateKeepVocalFile(newValue)
        setKeepVocalFile(newValue)
    }

    const onStartProcessing = async () => {
        await startProcessing()
    }

    const getProgressText = () => {
        if (state === "separating") {
            return "Separating Vocals..."
        } else if (state === "chopping") {
            return "Chopping Vocals..."
        } else {
            return "Waiting..."
        }
    }

    const cancelEvent = (event: DragEvent) => {
        event.preventDefault()
    }

    const dropEvent = async (event: DragEvent) => {
        event.preventDefault()
        if (!event.dataTransfer?.files.length) return
        const file = event.dataTransfer.files[0]
        const accepted = [".wav", ".mp3", ".ogg", ".flac"]
        if (accepted.some(ext => file.name.endsWith(ext))) {
            const buffer = await file.arrayBuffer()
            window.__JUCE__.backend.emitEvent("file-dropped", {
                name: file.name,
                data: Object.values(new Uint8Array(buffer))
            })
            setAudioPath(`[dropped file] ${file.name}`)
        }
    }

    const folderOpen = async () => {
        openFolder(destPath)
    }

    const showFile = async () => {
        showFileInFolder(audioPath)
    }

    return (
        <div className="app">
            <div className="column">
                <img src={logo} className="logo" draggable={false}/>
            </div>
            <div className="settings">
                <div className="column">
                    <input className="input" type="text" value={audioPath} placeholder="Select audio file..." readOnly onDoubleClick={showFile}></input>
                    <button className="button" onClick={onSelectAudio}><span>Select Audio</span></button>
                </div>
                <div className="column-start">
                    <input id="checkbox" className="checkbox-input" type="checkbox" checked={skipVocalExtraction} onClick={toggleSkipVocalExtraction}/>
                    <label htmlFor="checkbox" className="checkbox-square"></label>
                    <span style={{marginRight: "1rem"}} className="checkbox-text">Skip Vocal Extraction</span>
                    <input id="checkbox2" className="checkbox-input" type="checkbox" checked={keepVocalFile} onClick={toggleKeepVocalFile}/>
                    <label htmlFor="checkbox2" className="checkbox-square"></label>
                    <span className="checkbox-text">Keep Vocal File</span>
                </div>
                <div className="column">
                    <input className="input" type="text" value={destPath} placeholder="Select destination folder..." readOnly onDoubleClick={folderOpen}></input>
                    <button className="button" onClick={onSelectDest}><span>Select Dest</span></button>
                </div>
            </div>
            <div className="column">
                <button className={state ? "button-big-alt" : "button-big"} onClick={onStartProcessing}><span>{state ? "Stop" : "Generate"}</span></button>
            </div>
            <div className="column">
                <div className="progress-container">
                    <span className="progress-text">{getProgressText()}</span>
                    <ProgressBar animated now={progress}/>
                </div>
            </div>
        </div>
    )
}

const root = createRoot(document.getElementById("root")!)
root.render(<App/>)