const readline =  require('readline');
var chalk = require('chalk');
//var TEST_EXEC = './client';
//var TEST_EXEC = './test-server';
var TEST_EXEC = 'lwsws';
var spawn = require('child-process-promise').spawn;
var host = '192.168.1.100';
var port = '2565';
const rl = readline.createInterface({
    input: process.stdin,
    output: process.stdout,
    terminal: false
});

//var promise = spawn('stdbuf', ['-i0', '-o0', '-e0', TEST_EXEC,[`-h${host}`],[`-p${port}`]]);
var promise = spawn('stdbuf', ['-i0', '-o0', '-e0', TEST_EXEC ]);
var childProcess = promise.childProcess;

console.log('[spawn] childProcess.pid: ', childProcess.pid); 

childProcess.stdout.on('data', function (data) {
    console.log(chalk.green('[spawn] stdout: ' + data.toString())); // message from 
});
childProcess.stderr.on('data', function (data) {
    console.log(chalk.red('[spawn] stderr: ', data.toString()));
});
 

childProcess.on('exit', (code, signal) => {
    console.log(chalk.blue('[EXIT EVENT] child process exited with ' + `code: ${code} and signal ${signal}`));
});

childProcess.on('close', (closedCode) => {
    console.log(chalk.blue(`[CLOSE_EVENT] ${closedCode}`));
    rl.close();
});

childProcess.on('message', (message) => {
    console.log(chalk.blue(`[MESSAGE EVENT] ${message}`));
    rl.close();
});

process.on('SIGINT', () => {///ctrl + c
     console.log(chalk.yellow('Intercepting'));
     rl.close();
     return;
});


rl.on('line', function(line){
    if (line != '') {
        console.log(`childProcessPID: ${childProcess.pid}`);
        console.log(line);
        if(childProcess.pid !== 0) {
            childProcess.stdin.write(`${line}\n`);
        }
    }
})
    
promise.then(function (data) {
    // when process finished
    console.log('[promise] done!',data.childProcess.exitCode);
}).catch(function (err) {
    console.error(chalk.red('[promise] ERROR: ', err.code));
});
