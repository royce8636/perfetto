function readRtuxFile(file: File): Promise<Array<{key: number, value: string}>> {
    return new Promise((resolve, reject) => {
      const reader = new FileReader();
      reader.onload = () => {
        // Convert the result to a string
        const str = reader.result as string;
  
        // Parse the string into a vector
        const lines = str.split('\n');
        const vector = lines.map(line => {
          const [key, value] = line.split(': ');
          return {key: parseFloat(key), value};
        });
  
        resolve(vector);
      };
      reader.onerror = () => {
        reject(reader.error);
      };
      reader.readAsText(file);
    });
  }

  function openRtuxFromFile(file: File) {
    readRtuxFile(file).then(vector => {
      return vector;
    });
  }
  
  export const RTUX_common = {
    readRtuxFile,
    openRtuxFromFile,
  };