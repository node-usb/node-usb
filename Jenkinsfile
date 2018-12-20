pipeline {
  agent none
  options {
    skipDefaultCheckout()
    disableConcurrentBuilds()
    buildDiscarder(logRotator(numToKeepStr: '50', artifactNumToKeepStr: '50'))
  }
  parameters {
    booleanParam(name: 'ReleaseBuild', defaultValue: false, description: 'Should release the built platform')
  }
  stages {
    stage('Build') {
      failFast false
      parallel {
        stage('node 8 mac') {
          agent { label "iosBuild" }
          environment {
            NODE_VERSION="10.12.0"
            RELEASE="${params.ReleaseBuild}"
          }
          steps {
            echo "Running on $NODE_NAME"
            cleanWs()
            checkout scm

            withCredentials([string(credentialsId: 'azureHuddlySoftwareBlobStorageToken', variable: 'AZURE_STORAGE_ACCESS_KEY')]) {
               withEnv([
                 'AZURE_STORAGE_ACCOUNT=huddlysoftware'
               ]) {
                 sh('scripts/build_macos.sh');
               }
             }
          }
        }
        stage('node 8 linux') {
          agent { label "linux-workstation-oslo" }
          environment {
            NODE_VERSION="10.12.0"
            RELEASE="${params.ReleaseBuild}"
          }
          steps {
            echo "Running on $NODE_NAME"
            cleanWs()
            checkout scm

            withCredentials([string(credentialsId: 'azureHuddlySoftwareBlobStorageToken', variable: 'AZURE_STORAGE_ACCESS_KEY')]) {
               withEnv([
                 'AZURE_STORAGE_ACCOUNT=huddlysoftware'
               ]) {
                 sh('scripts/build_linux.sh')
               }
             }
          }
        }
        stage('node 8 win') {
          agent { label "win10-ci03" }
          environment {
            NODE_VERSION="10.12.0"
            RELEASE="${params.ReleaseBuild}"
          }
          steps {
            echo "Running on $NODE_NAME"
            cleanWs()
            checkout scm

            withCredentials([string(credentialsId: 'azureHuddlySoftwareBlobStorageToken', variable: 'AZURE_STORAGE_ACCESS_KEY')]) {
               withEnv([
                 'AZURE_STORAGE_ACCOUNT=huddlysoftware'
               ]) {
                 sh('scripts/build_win.sh')
               }
             }
          }
        }
      }
    }
  }
}
