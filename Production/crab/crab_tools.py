# Definition of tools for CRAB job submission.
# This file is part of https://github.com/hh-italian-group/h-tautau.

import sys
import re
from sets import Set
from CRABClient.UserUtilities import ClientException
from CRABAPI.RawCommand import crabCommand
from httplib import HTTPException

def submit(config, dryrunBool):
    try:
        crabCommand('submit', config = config, dryrun = dryrunBool)
    except HTTPException as hte:
        print str(hte)
        print "\n{}\nERROR: failed to submit task due to HTTPException.\n{}".format(hte, hte.headers)
    except ClientException as cle:
        print "ERROR: failed to submit task due to ClientException.\n{}".format(cle)

class Job:
    def __init__(self, line, jobNameSuffix = '', unitsPerJob = -1):
        items = filter(lambda s: len(s) != 0, re.split(" |\t", line))
        if len(items) != 3:
            print "ERROR: invalid job description = '{}'.".format(line)
            sys.exit(1)
        self.requestName = items[0] + jobNameSuffix
        if unitsPerJob == -1:
            self.unitsPerJob = int(items[1])
        else:
            self.unitsPerJob = unitsPerJob
        self.inputDataset = items[2]

    def __str__(self):
        return "requestName = '{}', unitsPerJob = {}, inputDataset = '{}'".format(self.requestName, self.unitsPerJob,
                                                                                  self.inputDataset)

    def submit(self, config, dryrunBool):
        config.General.requestName = self.requestName
        config.Data.inputDataset = self.inputDataset
        config.Data.unitsPerJob = self.unitsPerJob
        submit(config, dryrunBool)

class JobCollection:
    def __init__(self, file_name, job_names = '', lumi_mask = '', jobNameSuffix = '', unitsPerJob = -1):
        self.jobs = []
        self.jobNames = job_names
        input_file = open(file_name, 'r')
        lines = [ s.strip() for s in input_file.readlines() ]
        lines = filter(lambda s: len(s) != 0 and s[0] != '#', lines)
        if len(lines) <= 2:
            print "ERROR: file '{}' is empty".format(file_name)
            sys.exit(1)
        header_items = filter(lambda s: len(s) != 0, re.split(" |\n", lines[0]))
        self.pyCfgParams = filter(lambda s: len(s) != 0, re.split(" |\t", lines[1]))
        if len(header_items) == 0 or len(header_items) > 2:
            print "ERROR: invalid jobs file header '{}' in file '{}'".fromat(lines[0], file_name)
            sys.exit(1)
        self.splitting = header_items[0]
        known_splittings = Set(['FileBased', 'LumiBased', 'EventAwareLumiBased'])
        if not self.splitting in known_splittings:
            print "ERROR: unknown splitting = '{}' in file '{}'".format(self.splitting, file_name)
            sys.exit(1)
        self.lumiMask =  ''
        if len(header_items) > 1:
            if header_items[1].lower() == "signal":
                if len(lines) < 4:
                    print "ERROR: invalid signal jobs definition in file '{}'".format(file_name)
                    sys.exit(1)
                masses = filter(lambda s: len(s) != 0, re.split(" |\t", lines[2]))
                template = lines[3]
                for mass in masses:
                    line = template.format(M = mass)
                    self.jobs.append(Job(line))
                return
            else:
                self.lumiMask = header_items[1]
        if len(lumi_mask) != 0:
            self.lumiMask = lumi_mask

        for line in lines[2:]:
            self.jobs.append(Job(line, jobNameSuffix, unitsPerJob))
        input_file.close()

    def __str__(self):
        result = "Splitting = '{}', cfgParams = {}, lumiMask = '{}'".format(self.splitting, self.pyCfgParams,
                                                                              self.lumiMask)
        for job in self.jobs:
            result += "\n" + str(job)
        return result

    def submit(self, config, dryrunBool):
        config.Data.splitting = self.splitting
        config.JobType.pyCfgParams = self.pyCfgParams
        config.Data.lumiMask = self.lumiMask
        for job in self.jobs:
            if len(self.jobNames) == 0 or job.requestName in self.jobNames:
                job.submit(config, dryrunBool)
