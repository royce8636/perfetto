import {defer} from '../base/deferred';

enum WebContentScriptMessageType {
  UNKNOWN,
  CONVERT_OBJECT_URL,
  CONVERT_OBJECT_URL_RESPONSE,
}

const ANDROID_BUG_TOOL_EXTENSION_ID = 'mbbaofdfoekifkfpgehgffcpagbbjkmj';

interface Attachment {
  name: string;
  objectUrl: string;
  restrictionSeverity: number;
}

interface ConvertObjectUrlResponse {
  action: WebContentScriptMessageType.CONVERT_OBJECT_URL_RESPONSE;
  attachments: Attachment[];
  issueAccessLevel: string;
  issueId: string;
  issueTitle: string;
}

export interface TraceFromBuganizer {
  issueId: string;
  issueTitle: string;
  file: File;
}

export function loadAndroidBugToolInfo(): Promise<TraceFromBuganizer> {
  const deferred = defer<TraceFromBuganizer>();

  // Request to convert the blob object url "blob:chrome-extension://xxx"
  // the chrome extension has to a web downloadable url "blob:http://xxx".
  chrome.runtime.sendMessage(
      ANDROID_BUG_TOOL_EXTENSION_ID,
      {action: WebContentScriptMessageType.CONVERT_OBJECT_URL},
      async (response: ConvertObjectUrlResponse) => {
        switch (response.action) {
          case WebContentScriptMessageType.CONVERT_OBJECT_URL_RESPONSE:
          if (response.attachments?.length > 0) {
            const filesBlobPromises =
                response.attachments.map(async attachment => {
                  const fileQueryResponse = await fetch(attachment.objectUrl);
                  const blob = await fileQueryResponse.blob();
                  // Note: The blob's media type is always set to "image/png".
                  // Clone blob to clear media type.
                  return new File([blob], attachment.name);
                });
            const files = await Promise.all(filesBlobPromises);
            deferred.resolve({
              issueId: response.issueId,
              issueTitle: response.issueTitle,
              file: files[0],
            });
          } else {
            throw new Error('Got no attachements from extension');
          }
          break;
          default:
            throw new Error(`Received unhandled response code (${
                response.action}) from extension.`);
        }
      });
  return deferred;
}
