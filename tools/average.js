#!/usr/bin/env node
/* Get the query result files average, min and max */

let fs = require('fs');
let csvParser = require('csv-parser');
let csvWriter = require('csv-write-stream');

let dir = process.argv[2];
let SUBDIRS = [
    'queries-o-queries-highCardinality.txt',
    'queries-o-queries-lowCardinality.txt',
    'queries-p-queries-highCardinality.txt',
    'queries-p-queries-lowCardinality.txt',
    'queries-po-queries-highCardinality.txt',
    'queries-po-queries-lowCardinality.txt',
    'queries-s-queries-highCardinality.txt',
    'queries-s-queries-lowCardinality.txt',
    'queries-so-queries-lowCardinality.txt',
    'queries-sp-queries-highCardinality.txt',
    'queries-sp-queries-lowCardinality.txt',
    'queries-spo-queries.txt',
    'queries-p.txt',
    'queries-po.txt'
];

let columnsStaticVm = [
    'patch',
    'offset'
];
let columnsStaticDm = [
    'patch_start',
    'patch_end',
    'offset'
];
let columnsStaticVq = [
    'offset'
];
let columsStaticIndex = {
    'vm': columnsStaticVm,
    'dm': columnsStaticDm,
    'vq': columnsStaticVq,
};

SUBDIRS.forEach((subDir) => {
    fs.readdir(dir + subDir, (err, files) => {
        if (err) return;
        
        let medianIndex = {
            'vm': [],
            'dm': [],
            'vq': [],
        };
        let averageIndex = {
            'vm': [],
            'dm': [],
            'vq': [],
        };
        let minIndex = {
            'vm': [],
            'dm': [],
            'vq': [],
        };
        let maxIndex = {
            'vm': [],
            'dm': [],
            'vq': [],
        };
        let countIndex = {
            'vm': 0,
            'dm': 0,
            'vq': 0,
        };
        
        let type;
        
        let countdown = 0;
        files.forEach(file => {
            if (file.indexOf('versionmat-') == 0) {
                type = 'vm';
            } else if (file.indexOf('deltamat-') == 0) {
                type = 'dm';
            } else if (file.indexOf('version-') == 0) {
                type = 'vq';
            }
            if (type) {
            let medianData = medianIndex[type];
            let averageData = averageIndex[type];
            let minData = minIndex[type];
            let maxData = maxIndex[type];
            let columnsStatic = columsStaticIndex[type];
            countIndex[type]++;
            countdown++;
            file = dir + subDir + '/' + file;
            
            let row = 0;
            fs.createReadStream(file)
                .pipe(csvParser())
                .on('data', (data) => {
                    let medianRow = medianData[row];
                    let averageRow = averageData[row];
                    let minRow = minData[row];
                    let maxRow = maxData[row];
                    if (!averageRow) {
                        medianRow = (medianData[row] = {});
                        averageRow = (averageData[row] = {});
                        minRow = (minData[row] = {});
                        maxRow = (maxData[row] = {});
                    }
                    Object.keys(data).forEach((key) => {
                        let value = parseInt(data[key]);
                        if (columnsStatic.indexOf(key) >= 0) {
                            medianRow[key] = value;
                            averageRow[key] = value;
                            minRow[key] = value;
                            maxRow[key] = value;
                        } else {
                            if (!medianRow[key]) medianRow[key] = [];
                            medianRow[key].push(value);
                            
                            if (!averageRow[key]) averageRow[key] = 0;
                            averageRow[key] += value;
                            
                            if (!minRow[key]) minRow[key] = value;
                            else minRow[key] = Math.min(minRow[key], value);
                            
                            if (!maxRow[key]) maxRow[key] = value;
                            else maxRow[key] = Math.max(maxRow[key], value);
                        }
                    });
                    row++;
                })
                .on('end', () => {
                    if (--countdown === 0) {
                        console.log('end');
                        
                        ['vm', 'dm', 'vq'].forEach((type) => {
                            let writer = csvWriter();
                            writer.pipe(fs.createWriteStream(dir + subDir + '/_average_' + type + '.csv'));
                            averageIndex[type].forEach((row) => {
                                Object.keys(row).forEach((key) => {
                                    if (columsStaticIndex[type].indexOf(key) < 0) {
                                        row[key] /= countIndex[type];
                                        row[key] /= 1000;
                                    }
                                });                                
                                writer.write(row);
                            });
                            writer.end();
                            
                            writer = csvWriter();
                            writer.pipe(fs.createWriteStream(dir + subDir + '/_median_' + type + '.csv'));
                            medianIndex[type].forEach((row) => {
                                Object.keys(row).forEach((key) => {
                                    if (columsStaticIndex[type].indexOf(key) < 0) {
                                        row[key] = row[key].sort((a, b) => a - b)[Math.floor(row[key].length / 2)] / 1000;
                                    }
                                });
                                writer.write(row);
                            });
                            writer.end();
                            
                            writer = csvWriter();
                            writer.pipe(fs.createWriteStream(dir + subDir + '/_min_' + type + '.csv'));
                            minIndex[type].forEach((row) => {
                                Object.keys(row).forEach((key) => {
                                    if (columsStaticIndex[type].indexOf(key) < 0) {
                                        row[key] /= 1000;
                                    }
                                });  
                                writer.write(row)
                            });
                            writer.end();
                            
                            writer = csvWriter();
                            writer.pipe(fs.createWriteStream(dir + subDir + '/_max_' + type + '.csv'));
                            maxIndex[type].forEach((row) => {
                                Object.keys(row).forEach((key) => {
                                    if (columsStaticIndex[type].indexOf(key) < 0) {
                                        row[key] /= 1000;
                                    }
                                });  
                                writer.write(row)
                            });
                            writer.end();
                        });
                    }
                });
            }
        });
    })
});