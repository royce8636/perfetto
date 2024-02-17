
// Define a module-level variable to store the vector
let globalVector: Array<{ key: number, value: string }> = [];

function readRtuxFile(file: File): Promise<Array<{ key: number, value: string }>> {
    return new Promise((resolve, reject) => {
        const reader = new FileReader();
        reader.onload = () => {
            // Convert the result to a string
            const str = reader.result as string;

            // Parse the string into a vector
            const lines = str.split('\n');
            const vector = lines.map(line => {
                const [key, value] = line.split(': ');
                return { key: parseFloat(key), value };
            });

            // Store the vector in the global variable
            globalVector = vector;

            resolve(vector);
        };
        reader.onerror = () => {
            reject(reader.error);
        };
        reader.readAsText(file);
    });
}

// A new function to access the stored vector
function getStoredVector(): Array<{ key: number, value: string }> {
    return globalVector;
}

type Time = number;
function getTimestamps(): ArrayLike<Time>{
    const timestamps: Time[] = globalVector.map(item => item.key);
    // Creating an array-like object
    const arrayLikeTimestamps: ArrayLike<Time> = {
        length: timestamps.length,
        ...timestamps.reduce((acc, curr, index) => ({ ...acc, [index]: curr }), {}),
    };
    return arrayLikeTimestamps;
}

// Optionally, adjust the structure to encapsulate the operations
export const rtux_loader = {
    readRtuxFile,
    openRtuxFromFile: async (file: File) => {
        await readRtuxFile(file);
        // await pluginManager.onRtuxLoad();
        // You might want to return something or process the vector further here
    },
    getStoredVector, // Allow access to the stored vector
    getTimestamps,
};