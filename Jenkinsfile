def unixNVMUse = """\
#!/bin/bash -l
export NVM_DIR="\$HOME/.nvm"
[ -s "\$NVM_DIR/nvm.sh" ] && . "\$NVM_DIR/nvm.sh" # This loads nvm
nvm install 8.9.4
nvm use 8.9.4"""

def jobs = [
  [
    label: "win10-ci3",
    nodeName: "",
    commands: [ 
      'npm i',
      'npm run build'
    ],
    release: "npm run release-current-platform"
  ],
  [
    label: "linux-workstation-oslo",
    nodeName: "",
    commands: [
      """$unixNVMUse
      npm i""",
      """$unixNVMUse
      npm run build""",
    ],
    release: """$unixNVMUse
    npm run release-current-platform"""
  ],
  [
    label: "iosBuild",
    nodeName: "",
    commands: [
      """$unixNVMUse
      npm i --python=/usr/bin/python2.7""",
      """$unixNVMUse
      source activate py27
      npm run build""",
    ],
    release: """$unixNVMUse
    source activate py27
    npm run release-current-platform"""
  ]
]

properties([
  disableConcurrentBuilds(),
  parameters([
    booleanParam(name: 'ReleaseBuild', defaultValue: false, description: 'Should release the built platform'),
  ]),
  [$class: 'BuildDiscarderProperty', strategy: [ $class: 'LogRotator', artifactNumToKeepStr: '50', numToKeepStr: '50']]
])

stage('Checkout') {
  def builders = [:]

  for (x in jobs) {
    def job = x
    def label = job.label

    builders[label] = {
      node(label) {
        sshagent(credentials: ['a449d54a-b54e-47b7-8af8-c895965663e9']) {
          cleanWs()
          checkout([
            $class: 'GitSCM',
            branches: scm.branches,
            extensions: scm.extensions + [[$class: 'CleanCheckout']],
            userRemoteConfigs: [[url: 'git@github.com:Huddly/node-usb.git', credentialsId: 'a449d54a-b54e-47b7-8af8-c895965663e9']]
          ])
          job.nodeName = env.NODE_NAME
        }
      }
    }
  }

  parallel builders
}

stage('Install') {
  def builders = [:]

  for(x in jobs) {
    def job = x
    def label = job.nodeName
    builders[label] = {
      node(label) {
        timeout(time: 10, unit: "MINUTES") {
          try {
            sh 'git submodule update --init --recursive'
            sh(job.commands[0])
          } catch(Exception err) {
            currentBuild.result = 'FAILURE'
            //notifySlack('FAILURE', "NODE-USB job failed on ${stageName} stage! Node ${env.NODE_NAME}")
            error("NODE-USB job failed on Install stage! Node ${env.NODE_NAME}")
          }
        }
      }
    }
  }
  parallel builders
}

stage('Build') {
  def builders = [:]

  for(x in jobs) {
    def job = x
    def label = job.nodeName
    builders[label] = {
      node(label) {
        timeout(time: 10, unit: "MINUTES") {
          try {
            sh(job.commands[1])
          } catch(Exception err) {
            currentBuild.result = 'FAILURE'
            //notifySlack('FAILURE', "NODE-USB job failed on ${stageName} stage! Node ${env.NODE_NAME}")
            error("NODE-USB job failed on Build stage! Node ${env.NODE_NAME}")
          }
        }
      }
    }
  }
  parallel builders
}

if(params.ReleaseBuild) {
  stage('Release Build') {
    def builders = [:]

    for (x in jobs) {
      def job = x
      def label = job.nodeName

      builders[label] = {
        node(label) {
          timeout(time: 10, unit: "MINUTES") {
            try {
              withCredentials([string(credentialsId: 'azureHuddlySoftwareBlobStorageToken', variable: 'AZURE_STORAGE_ACCESS_KEY')]) {
                withEnv([
                  'AZURE_STORAGE_ACCOUNT=huddlysoftware'
                ]) {
                  sh(job.release)
                }
              }
            } catch(Exception err) {
              currentBuild.result = 'FAILURE'
              //notifySlack('FAILURE', "NODE-USB job failed on Release stage! Node ${env.NODE_NAME}")
              error("NODE-USB job failed on Release stage! Node ${env.NODE_NAME}")
            }
          }
        }
      }
    }
    parallel builders
  }
}

def notifySlack(String buildStatus, String inStage = '') {

  def colorName = 'RED'
  def colorCode = '#FF0000'
  def subject = "${buildStatus}: Job '${env.JOB_NAME} [${env.BUILD_NUMBER}]'"
  def summary = "${subject} (${env.BUILD_URL})"
  def emoji = "";
  def stageDesc = ""

  /* Override default values based on build status */
  if (buildStatus == 'STARTED') {
    color = 'YELLOW'
    colorCode = '#FFFF00'
  } else if (buildStatus == 'SUCCESS' ) {
    color = 'GREEN'
    colorCode = '#00FF00'
  } else {
    color = 'RED'
    colorCode = '#FF0000'
    stageDesc = "STAGE: ${inStage}"
  }

  /* Send Slack notifications */
//  slackSend (channel: '#electron-app', color: colorCode, message: "${summary} ${stageDesc}", teamDomain: <team domain>, token: <token>)
}