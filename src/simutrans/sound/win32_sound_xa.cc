/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

 /* The code is mostly copied and slightly adapted straight from the microsoft documetnation.
  * There was no clear indicating of license, so public domain is assumed.
  */

#include <windows.h>

#if defined (__GNUC__)
#include <xaudio2.h>
#else
#include <xaudio2.h>
#endif

#include "../tpl/vector_tpl.h"

vector_tpl<WAVEFORMATEXTENSIBLE> wfs;
vector_tpl<XAUDIO2_BUFFER > bufs;

static IXAudio2* pXAudio2 = NULL;
static IXAudio2MasteringVoice* pMasterVoice = NULL;

/**
 * Sound initialisation routine
 */
bool dr_init_sound()
{
    if( XAudio2Create(&pXAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR) < 0) {
        return false;
    }
    if( pXAudio2->CreateMasteringVoice(&pMasterVoice) < 0 ) {
        return false;
    }
    return true;
}

#ifdef SIM_BIG_ENDIAN
#define fourccRIFF 'RIFF'
#define fourccDATA 'data'
#define fourccFMT 'fmt '
#define fourccWAVE 'WAVE'
#define fourccXWMA 'XWMA'
#define fourccDPDS 'dpds'
#else
#define fourccRIFF 'FFIR'
#define fourccDATA 'atad'
#define fourccFMT ' tmf'
#define fourccWAVE 'EVAW'
#define fourccXWMA 'AMWX'
#define fourccDPDS 'sdpd'
#endif
HRESULT FindChunk(HANDLE hFile, DWORD fourcc, DWORD& dwChunkSize, DWORD& dwChunkDataPosition)
{
    HRESULT hr = S_OK;
    if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, 0, NULL, FILE_BEGIN))
        return HRESULT_FROM_WIN32(GetLastError());

    DWORD dwChunkType;
    DWORD dwChunkDataSize;
    DWORD dwRIFFDataSize = 0;
    DWORD dwFileType;
    DWORD bytesRead = 0;
    DWORD dwOffset = 0;

    while (hr == S_OK)
    {
        DWORD dwRead;
        if (0 == ReadFile(hFile, &dwChunkType, sizeof(DWORD), &dwRead, NULL))
            hr = HRESULT_FROM_WIN32(GetLastError());

        if (0 == ReadFile(hFile, &dwChunkDataSize, sizeof(DWORD), &dwRead, NULL))
            hr = HRESULT_FROM_WIN32(GetLastError());

        switch (dwChunkType)
        {
        case fourccRIFF:
            dwRIFFDataSize = dwChunkDataSize;
            dwChunkDataSize = 4;
            if (0 == ReadFile(hFile, &dwFileType, sizeof(DWORD), &dwRead, NULL))
                hr = HRESULT_FROM_WIN32(GetLastError());
            break;

        default:
            if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, dwChunkDataSize, NULL, FILE_CURRENT))
                return HRESULT_FROM_WIN32(GetLastError());
        }

        dwOffset += sizeof(DWORD) * 2;

        if (dwChunkType == fourcc)
        {
            dwChunkSize = dwChunkDataSize;
            dwChunkDataPosition = dwOffset;
            return S_OK;
        }

        dwOffset += dwChunkDataSize;

        if (bytesRead >= dwRIFFDataSize) return S_FALSE;

    }

    return S_OK;

}

static HRESULT ReadChunkData(HANDLE hFile, void* buffer, DWORD buffersize, DWORD bufferoffset)
{
    HRESULT hr = S_OK;
    if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, bufferoffset, NULL, FILE_BEGIN))
        return HRESULT_FROM_WIN32(GetLastError());
    DWORD dwRead;
    if (0 == ReadFile(hFile, buffer, buffersize, &dwRead, NULL))
        hr = HRESULT_FROM_WIN32(GetLastError());
    return hr;
}

/**
 * loads a single sample
 * @return a handle for that sample or -1 on failure
 */
int dr_load_sample(char const* strFileName)
{
    WAVEFORMATEXTENSIBLE wfx = { 0 };
    XAUDIO2_BUFFER buffer = { 0 };
    // Open the file
    HANDLE hFile = CreateFileA(strFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

    if (INVALID_HANDLE_VALUE == hFile) {
        return -1;
	}

    if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, 0, NULL, FILE_BEGIN)) {
        return -1;
	}

    DWORD dwChunkSize;
    DWORD dwChunkPosition;

    //check the file type, should be fourccWAVE or 'XWMA'
    FindChunk(hFile, fourccRIFF, dwChunkSize, dwChunkPosition);
    DWORD filetype;
    ReadChunkData(hFile, &filetype, sizeof(DWORD), dwChunkPosition);
    if (filetype != fourccWAVE) {
        return -1;
	}
    FindChunk(hFile, fourccFMT, dwChunkSize, dwChunkPosition);
    ReadChunkData(hFile, &wfx, dwChunkSize, dwChunkPosition);
    //fill out the audio data buffer with the contents of the fourccDATA chunk
    FindChunk(hFile, fourccDATA, dwChunkSize, dwChunkPosition);
    BYTE* pDataBuffer = new BYTE[dwChunkSize];
    ReadChunkData(hFile, pDataBuffer, dwChunkSize, dwChunkPosition);
    buffer.AudioBytes = dwChunkSize;  //size of the audio buffer in bytes
    buffer.pAudioData = pDataBuffer;  //buffer containing audio data
    buffer.Flags = XAUDIO2_END_OF_STREAM; // tell the source voice not to expect any data after this buffer

    wfs.append(wfx);
    bufs.append(buffer);
    return wfs.get_count() - 1;
}

/**
 * plays a sample
 * @param key the key for the sample to be played
 */
void dr_play_sample(int sample_number, int volume)
{
    IXAudio2SourceVoice* pSourceVoice;
    if (pXAudio2->CreateSourceVoice(&pSourceVoice, (WAVEFORMATEX*)&(wfs[sample_number]))<0  ||pSourceVoice->SubmitSourceBuffer(&(bufs[sample_number])) < 0) {
        return;
	}
    pSourceVoice->SetVolume(volume/256.0);
    pSourceVoice->Start(0);
}
